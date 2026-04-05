"""
API routes for pump control and device status
"""

from datetime import datetime, timedelta
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from ..database import get_db, DeviceStatus as DeviceStatusModel, PumpEvent
from ..schemas import PumpControlRequest, PumpControlResponse, DeviceStatus
from ..mqtt_client import get_mqtt_client

router = APIRouter(tags=["control"])


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
