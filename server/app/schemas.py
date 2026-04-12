"""
Pydantic schemas for API request/response validation
"""

from datetime import datetime
from pydantic import BaseModel, Field
from typing import Optional, Literal


# ============================================================================
# Sensor Schemas
# ============================================================================

class SensorReadingBase(BaseModel):
    """Base sensor reading schema"""
    temperature: float = Field(..., description="Temperature in Celsius")
    humidity: float = Field(..., description="Humidity percentage")


class SensorReading(SensorReadingBase):
    """Sensor reading with timestamp"""
    id: int
    timestamp: datetime
    
    class Config:
        from_attributes = True


class SensorReadingCreate(SensorReadingBase):
    """Schema for creating a sensor reading"""
    pass


# ============================================================================
# Schedule Schemas
# ============================================================================

class ScheduleBase(BaseModel):
    """Base schedule schema"""
    hour: int = Field(..., ge=0, le=23, description="Hour (0-23)")
    minute: int = Field(..., ge=0, le=59, description="Minute (0-59)")
    duration_min: int = Field(5, ge=1, le=120, description="Duration in minutes")
    enabled: bool = True


class Schedule(ScheduleBase):
    """Schedule with ID and timestamps"""
    id: int
    created_at: datetime
    updated_at: datetime
    
    class Config:
        from_attributes = True


class ScheduleCreate(ScheduleBase):
    """Schema for creating a schedule"""
    pass


class ScheduleUpdate(BaseModel):
    """Schema for updating schedules (list of schedules)"""
    schedules: list[ScheduleCreate]


# ============================================================================
# Pump Control Schemas
# ============================================================================

class PumpControlRequest(BaseModel):
    """Schema for pump control commands"""
    action: Literal["on", "off", "auto"] = Field(..., description="Pump action")
    duration_min: Optional[int] = Field(None, ge=1, le=120, description="Duration for 'on' action")


class PumpControlResponse(BaseModel):
    """Response from pump control command"""
    success: bool
    action: str
    message: str


# ============================================================================
# Status Schemas
# ============================================================================

class DeviceStatus(BaseModel):
    """ESP32 device status"""
    pump_state: str
    wifi_rssi: int
    uptime_sec: int
    free_heap: int
    last_seen: datetime
    connected: bool
    
    class Config:
        from_attributes = True


# ============================================================================
# Timezone Schemas
# ============================================================================

class TimezoneConfig(BaseModel):
    """Timezone configuration"""
    timezone: str = Field(..., description="IANA timezone name (e.g., 'Australia/Sydney', 'America/New_York')")
    gmt_offset_sec: Optional[int] = Field(None, description="GMT offset in seconds (calculated from timezone)")
    daylight_offset_sec: int = Field(0, description="Daylight saving offset in seconds")


class TimezoneUpdate(BaseModel):
    """Schema for updating timezone"""
    timezone: str = Field(..., description="IANA timezone name")


# ============================================================================
# Query Schemas
# ============================================================================

class SensorQuery(BaseModel):
    """Query parameters for sensor readings"""
    start_date: Optional[datetime] = None
    end_date: Optional[datetime] = None
    limit: int = Field(100, ge=1, le=1000)
