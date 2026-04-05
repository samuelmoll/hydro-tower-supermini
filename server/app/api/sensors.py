"""
API routes for sensor data
"""

from datetime import datetime, timedelta
from typing import Optional
from fastapi import APIRouter, Depends, Query
from sqlalchemy.orm import Session
from sqlalchemy import desc

from ..database import get_db, SensorReading as SensorReadingModel
from ..schemas import SensorReading

router = APIRouter(prefix="/sensors", tags=["sensors"])


@router.get("", response_model=list[SensorReading])
def get_sensor_readings(
    start_date: Optional[datetime] = Query(None, description="Start date filter"),
    end_date: Optional[datetime] = Query(None, description="End date filter"),
    limit: int = Query(100, ge=1, le=1000, description="Maximum records to return"),
    db: Session = Depends(get_db)
):
    """
    Get sensor readings with optional date range filter.
    
    Returns readings in reverse chronological order (newest first).
    """
    query = db.query(SensorReadingModel)
    
    # Apply date filters
    if start_date:
        query = query.filter(SensorReadingModel.timestamp >= start_date)
    if end_date:
        query = query.filter(SensorReadingModel.timestamp <= end_date)
    
    # Order by timestamp descending and limit
    readings = query.order_by(desc(SensorReadingModel.timestamp)).limit(limit).all()
    
    return readings


@router.get("/latest", response_model=Optional[SensorReading])
def get_latest_reading(db: Session = Depends(get_db)):
    """
    Get the most recent sensor reading.
    """
    reading = db.query(SensorReadingModel).order_by(
        desc(SensorReadingModel.timestamp)
    ).first()
    
    return reading


@router.get("/stats")
def get_sensor_stats(
    hours: int = Query(24, ge=1, le=168, description="Number of hours to analyze"),
    db: Session = Depends(get_db)
):
    """
    Get sensor statistics for the specified time period.
    
    Returns min, max, and average for temperature and humidity.
    """
    from sqlalchemy import func
    
    cutoff = datetime.utcnow() - timedelta(hours=hours)
    
    stats = db.query(
        func.min(SensorReadingModel.temperature).label("temp_min"),
        func.max(SensorReadingModel.temperature).label("temp_max"),
        func.avg(SensorReadingModel.temperature).label("temp_avg"),
        func.min(SensorReadingModel.humidity).label("humidity_min"),
        func.max(SensorReadingModel.humidity).label("humidity_max"),
        func.avg(SensorReadingModel.humidity).label("humidity_avg"),
        func.count(SensorReadingModel.id).label("reading_count")
    ).filter(SensorReadingModel.timestamp >= cutoff).first()
    
    return {
        "period_hours": hours,
        "reading_count": stats.reading_count or 0,
        "temperature": {
            "min": round(stats.temp_min, 1) if stats.temp_min else None,
            "max": round(stats.temp_max, 1) if stats.temp_max else None,
            "avg": round(stats.temp_avg, 1) if stats.temp_avg else None
        },
        "humidity": {
            "min": round(stats.humidity_min, 1) if stats.humidity_min else None,
            "max": round(stats.humidity_max, 1) if stats.humidity_max else None,
            "avg": round(stats.humidity_avg, 1) if stats.humidity_avg else None
        }
    }
