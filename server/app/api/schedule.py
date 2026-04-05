"""
API routes for pump schedule management
"""

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from datetime import datetime

from ..database import get_db, Schedule as ScheduleModel
from ..schemas import Schedule, ScheduleCreate, ScheduleUpdate
from ..mqtt_client import get_mqtt_client

router = APIRouter(prefix="/schedule", tags=["schedule"])


@router.get("", response_model=list[Schedule])
def get_schedules(db: Session = Depends(get_db)):
    """
    Get all pump schedules.
    """
    schedules = db.query(ScheduleModel).order_by(
        ScheduleModel.hour, ScheduleModel.minute
    ).all()
    return schedules


@router.put("", response_model=list[Schedule])
def update_schedules(
    schedule_update: ScheduleUpdate,
    db: Session = Depends(get_db)
):
    """
    Replace all schedules with given list.
    
    This will delete existing schedules and create new ones.
    The updated schedule is also published to the ESP32 via MQTT.
    """
    # Delete existing schedules
    db.query(ScheduleModel).delete()
    
    # Create new schedules
    new_schedules = []
    for sched in schedule_update.schedules:
        db_schedule = ScheduleModel(
            hour=sched.hour,
            minute=sched.minute,
            duration_min=sched.duration_min,
            enabled=sched.enabled
        )
        db.add(db_schedule)
        new_schedules.append(db_schedule)
    
    db.commit()
    
    # Refresh to get IDs
    for sched in new_schedules:
        db.refresh(sched)
    
    # Publish to ESP32 via MQTT
    mqtt_client = get_mqtt_client()
    if mqtt_client.connected:
        mqtt_schedules = [
            {
                "hour": s.hour,
                "minute": s.minute,
                "duration_min": s.duration_min,
                "enabled": s.enabled
            }
            for s in new_schedules
        ]
        mqtt_client.publish_schedule(mqtt_schedules)
    
    return new_schedules


@router.post("", response_model=Schedule)
def add_schedule(
    schedule: ScheduleCreate,
    db: Session = Depends(get_db)
):
    """
    Add a single schedule entry.
    """
    db_schedule = ScheduleModel(
        hour=schedule.hour,
        minute=schedule.minute,
        duration_min=schedule.duration_min,
        enabled=schedule.enabled
    )
    db.add(db_schedule)
    db.commit()
    db.refresh(db_schedule)
    
    # Publish updated schedules to ESP32
    _sync_schedules_to_esp32(db)
    
    return db_schedule


@router.delete("/{schedule_id}")
def delete_schedule(
    schedule_id: int,
    db: Session = Depends(get_db)
):
    """
    Delete a schedule entry by ID.
    """
    schedule = db.query(ScheduleModel).filter(ScheduleModel.id == schedule_id).first()
    if not schedule:
        raise HTTPException(status_code=404, detail="Schedule not found")
    
    db.delete(schedule)
    db.commit()
    
    # Publish updated schedules to ESP32
    _sync_schedules_to_esp32(db)
    
    return {"message": "Schedule deleted"}


def _sync_schedules_to_esp32(db: Session):
    """Helper to sync all schedules to ESP32 via MQTT"""
    mqtt_client = get_mqtt_client()
    if mqtt_client.connected:
        schedules = db.query(ScheduleModel).all()
        mqtt_schedules = [
            {
                "hour": s.hour,
                "minute": s.minute,
                "duration_min": s.duration_min,
                "enabled": s.enabled
            }
            for s in schedules
        ]
        mqtt_client.publish_schedule(mqtt_schedules)
