"""
API routes for pump control and device status
"""

from datetime import datetime, timedelta
from zoneinfo import ZoneInfo
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from ..config import get_settings
from ..database import get_db, DeviceStatus as DeviceStatusModel, PumpEvent
from ..schemas import PumpControlRequest, PumpControlResponse, DeviceStatus, TimezoneConfig, TimezoneUpdate
from ..mqtt_client import get_mqtt_client

router = APIRouter(tags=["control"])
settings = get_settings()


@router.post("/pump/control", response_model=PumpControlResponse)
def control_pump(request: PumpControlRequest):
    """
    Send pump control command to ESP32.
    
    Actions:
    - `on`: Turn pump on (optionally with duration_min)
    - `off`: Turn pump off
    - `auto`: Set pump to automatic/scheduled mode
    """
    mqtt_client = get_mqtt_client()
    
    if not mqtt_client.connected:
        raise HTTPException(
            status_code=503,
            detail="MQTT broker not connected"
        )
    
    success = mqtt_client.publish_pump_control(
        request.action,
        request.duration_min
    )
    
    if success:
        message = f"Pump command '{request.action}' sent successfully"
        if request.duration_min:
            message += f" (duration: {request.duration_min} min)"
    else:
        message = "Failed to publish pump command"
    
    return PumpControlResponse(
        success=success,
        action=request.action,
        message=message
    )


@router.get("/status", response_model=DeviceStatus)
def get_device_status(db: Session = Depends(get_db)):
    """
    Get current ESP32 device status.
    
    Includes pump state, WiFi signal strength, uptime, and connection status.
    """
    status = db.query(DeviceStatusModel).first()
    
    if not status:
        raise HTTPException(
            status_code=404,
            detail="No device status available. ESP32 may not be connected."
        )
    
    # Check if device is still connected (last seen within 10 minutes)
    connected = (datetime.utcnow() - status.last_seen) < timedelta(minutes=10)
    
    return DeviceStatus(
        pump_state=status.pump_state,
        wifi_rssi=status.wifi_rssi,
        uptime_sec=status.uptime_sec,
        free_heap=status.free_heap,
        last_seen=status.last_seen,
        connected=connected
    )


@router.get("/pump/history")
def get_pump_history(
    limit: int = 50,
    db: Session = Depends(get_db)
):
    """
    Get pump event history.
    
    Returns recent pump on/off events with timestamps.
    """
    from sqlalchemy import desc
    
    events = db.query(PumpEvent).order_by(
        desc(PumpEvent.timestamp)
    ).limit(limit).all()
    
    return [
        {
            "id": e.id,
            "action": e.action,
            "source": e.source,
            "timestamp": e.timestamp.isoformat()
        }
        for e in events
    ]


@router.post("/sensor/config")
def update_sensor_config(interval_sec: int):
    """
    Update sensor reading interval on ESP32.
    
    Valid range: 10-3600 seconds.
    """
    if interval_sec < 10 or interval_sec > 3600:
        raise HTTPException(
            status_code=400,
            detail="Interval must be between 10 and 3600 seconds"
        )
    
    mqtt_client = get_mqtt_client()
    
    if not mqtt_client.connected:
        raise HTTPException(
            status_code=503,
            detail="MQTT broker not connected"
        )
    
    success = mqtt_client.publish_sensor_config(interval_sec)
    
    return {
        "success": success,
        "interval_sec": interval_sec,
        "message": f"Sensor interval set to {interval_sec} seconds"
    }


def get_timezone_offset(timezone_name: str) -> int:
    """Get GMT offset in seconds for a timezone"""
    try:
        tz = ZoneInfo(timezone_name)
        # Get the current offset (accounts for DST automatically)
        now = datetime.now(tz)
        offset = now.utcoffset()
        if offset is None:
            return 0
        return int(offset.total_seconds())
    except Exception:
        return 0


@router.get("/timezone", response_model=TimezoneConfig)
def get_timezone():
    """
    Get current timezone configuration.
    
    Returns the configured timezone and calculated GMT offset.
    """
    gmt_offset = get_timezone_offset(settings.timezone)
    
    return TimezoneConfig(
        timezone=settings.timezone,
        gmt_offset_sec=gmt_offset,
        daylight_offset_sec=0  # ZoneInfo handles DST in the offset
    )


@router.put("/timezone", response_model=TimezoneConfig)
def update_timezone(request: TimezoneUpdate):
    """
    Update timezone configuration and send to ESP32.
    
    Accepts IANA timezone names like 'Australia/Sydney', 'America/New_York', etc.
    The timezone is converted to GMT offset in seconds for the ESP32.
    """
    # Validate timezone
    try:
        ZoneInfo(request.timezone)
    except Exception:
        raise HTTPException(
            status_code=400,
            detail=f"Invalid timezone: {request.timezone}. Use IANA timezone format (e.g., 'America/New_York')"
        )
    
    # Calculate offsets
    gmt_offset = get_timezone_offset(request.timezone)
    
    # Update settings (note: this updates the cached settings, not the env file)
    # For persistent storage, you'd need to save to a database or config file
    settings.timezone = request.timezone
    
    # Publish to ESP32
    mqtt_client = get_mqtt_client()
    
    if mqtt_client.connected:
        mqtt_client.publish_timezone(gmt_offset, 0)
    
    return TimezoneConfig(
        timezone=request.timezone,
        gmt_offset_sec=gmt_offset,
        daylight_offset_sec=0
    )
