"""
Database models and connection for Hydroponic Tower
"""

from datetime import datetime
from sqlalchemy import create_engine, Column, Integer, Float, String, DateTime, Boolean
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, Session
from typing import Generator

from .config import get_settings

settings = get_settings()

# Create database engine
engine = create_engine(
    settings.database_url,
    connect_args={"check_same_thread": False}  # SQLite specific
)

# Session factory
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

# Base class for models
Base = declarative_base()


class SensorReading(Base):
    """Sensor reading from ESP32"""
    __tablename__ = "sensor_readings"
    
    id = Column(Integer, primary_key=True, index=True)
    temperature = Column(Float, nullable=False)
    humidity = Column(Float, nullable=False)
    timestamp = Column(DateTime, default=datetime.utcnow, index=True)


class PumpEvent(Base):
    """Pump state change event"""
    __tablename__ = "pump_events"
    
    id = Column(Integer, primary_key=True, index=True)
    action = Column(String(20), nullable=False)  # on, off, auto
    source = Column(String(20), nullable=False)  # mqtt, api, schedule
    timestamp = Column(DateTime, default=datetime.utcnow, index=True)


class DeviceStatus(Base):
    """Latest device status"""
    __tablename__ = "device_status"
    
    id = Column(Integer, primary_key=True)
    pump_state = Column(String(10))
    wifi_rssi = Column(Integer)
    uptime_sec = Column(Integer)
    free_heap = Column(Integer)
    last_seen = Column(DateTime, default=datetime.utcnow)


class Schedule(Base):
    """Pump schedule configuration"""
    __tablename__ = "schedules"
    
    id = Column(Integer, primary_key=True, index=True)
    hour = Column(Integer, nullable=False)
    minute = Column(Integer, nullable=False)
    duration_min = Column(Integer, nullable=False, default=5)
    enabled = Column(Boolean, default=True)
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)


def create_tables():
    """Create all database tables"""
    Base.metadata.create_all(bind=engine)


def get_db() -> Generator[Session, None, None]:
    """Dependency for getting database session"""
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
