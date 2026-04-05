"""
MQTT Client for communication with ESP32
"""

import json
import logging
from datetime import datetime, timezone
from typing import Callable, Optional
import paho.mqtt.client as mqtt
from sqlalchemy.orm import Session

from .config import get_settings
from .database import SessionLocal, SensorReading, DeviceStatus, PumpEvent

settings = get_settings()
logger = logging.getLogger(__name__)


class MQTTClient:
    """MQTT client for ESP32 communication"""
    
    def __init__(self):
        self.client = mqtt.Client(
            client_id=settings.mqtt_client_id,
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2
        )
        self.connected = False
        self._on_sensor_callback: Optional[Callable] = None
        self._on_status_callback: Optional[Callable] = None
        
        # Set up callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        
        # Set credentials if provided
        if settings.mqtt_username:
            self.client.username_pw_set(
                settings.mqtt_username,
                settings.mqtt_password
            )
    
    def connect(self):
        """Connect to MQTT broker"""
        try:
            logger.info(f"Connecting to MQTT broker at {settings.mqtt_broker}:{settings.mqtt_port}")
            self.client.connect(settings.mqtt_broker, settings.mqtt_port, keepalive=60)
            self.client.loop_start()
        except Exception as e:
            logger.error(f"Failed to connect to MQTT broker: {e}")
            raise
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        self.client.loop_stop()
        self.client.disconnect()
    
    def _on_connect(self, client, userdata, flags, reason_code, properties):
        """Callback when connected to broker"""
        if reason_code == 0:
            logger.info("Connected to MQTT broker")
            self.connected = True
            
            # Subscribe to ESP32 topics
            client.subscribe(settings.mqtt_topic_sensors)
            client.subscribe(settings.mqtt_topic_status)
            logger.info(f"Subscribed to {settings.mqtt_topic_sensors} and {settings.mqtt_topic_status}")
        else:
            logger.error(f"Failed to connect to MQTT broker: {reason_code}")
    
    def _on_disconnect(self, client, userdata, flags, reason_code, properties):
        """Callback when disconnected from broker"""
        logger.warning(f"Disconnected from MQTT broker: {reason_code}")
        self.connected = False
    
    def _on_message(self, client, userdata, msg):
        """Callback for incoming messages"""
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            logger.debug(f"Received message on {topic}: {payload}")
            
            # Route message to appropriate handler
            if topic == settings.mqtt_topic_sensors:
                self._handle_sensor_data(payload)
            elif topic == settings.mqtt_topic_status:
                self._handle_status_data(payload)
                
        except json.JSONDecodeError as e:
            logger.error(f"Failed to parse MQTT message: {e}")
        except Exception as e:
            logger.error(f"Error handling MQTT message: {e}")
    
    def _handle_sensor_data(self, data: dict):
        """Store sensor reading in database"""
        db = SessionLocal()
        try:
            reading = SensorReading(
                temperature=data.get("temperature", 0),
                humidity=data.get("humidity", 0),
                timestamp=datetime.now(timezone.utc)
            )
            db.add(reading)
            db.commit()
            logger.debug(f"Stored sensor reading: {data}")
            
            if self._on_sensor_callback:
                self._on_sensor_callback(data)
                
        finally:
            db.close()
    
    def _handle_status_data(self, data: dict):
        """Update device status in database"""
        db = SessionLocal()
        try:
            # Upsert device status (single row)
            status = db.query(DeviceStatus).first()
            if status is None:
                status = DeviceStatus(id=1)
                db.add(status)
            
            status.pump_state = data.get("pump_state", "unknown")
            status.wifi_rssi = data.get("wifi_rssi", 0)
            status.uptime_sec = data.get("uptime_sec", 0)
            status.free_heap = data.get("free_heap", 0)
            status.last_seen = datetime.now(timezone.utc)
            
            db.commit()
            logger.debug(f"Updated device status: {data}")
            
            if self._on_status_callback:
                self._on_status_callback(data)
                
        finally:
            db.close()
    
    def publish_pump_control(self, action: str, duration_min: Optional[int] = None):
        """Send pump control command to ESP32"""
        payload = {"action": action}
        if duration_min is not None:
            payload["duration_min"] = duration_min
        
        result = self.client.publish(
            settings.mqtt_topic_pump_control,
            json.dumps(payload)
        )
        
        # Log pump event
        db = SessionLocal()
        try:
            event = PumpEvent(action=action, source="api")
            db.add(event)
            db.commit()
        finally:
            db.close()
        
        return result.is_published()
    
    def publish_schedule(self, schedules: list[dict]):
        """Send schedule configuration to ESP32"""
        payload = {"schedules": schedules}
        result = self.client.publish(
            settings.mqtt_topic_schedule_config,
            json.dumps(payload)
        )
        return result.is_published()
    
    def publish_sensor_config(self, interval_sec: int):
        """Send sensor interval configuration to ESP32"""
        payload = {"interval_sec": interval_sec}
        result = self.client.publish(
            settings.mqtt_topic_sensor_config,
            json.dumps(payload)
        )
        return result.is_published()
    
    def on_sensor(self, callback: Callable):
        """Register callback for sensor readings"""
        self._on_sensor_callback = callback
    
    def on_status(self, callback: Callable):
        """Register callback for status updates"""
        self._on_status_callback = callback


# Global MQTT client instance
mqtt_client: Optional[MQTTClient] = None


def get_mqtt_client() -> MQTTClient:
    """Get the global MQTT client instance"""
    global mqtt_client
    if mqtt_client is None:
        mqtt_client = MQTTClient()
    return mqtt_client
