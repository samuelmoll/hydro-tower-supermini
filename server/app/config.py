"""
Hydroponic Tower Backend Configuration
"""

from pydantic_settings import BaseSettings
from functools import lru_cache


class Settings(BaseSettings):
    """Application settings loaded from environment variables"""
    
    # Application
    app_name: str = "Hydroponic Tower API"
    debug: bool = True
    
    # MQTT Broker
    mqtt_broker: str = "localhost"
    mqtt_port: int = 1883
    mqtt_username: str = "hydro"
    mqtt_password: str = "hydro_secret"
    mqtt_client_id: str = "hydro_server"
    
    # MQTT Topics
    mqtt_topic_sensors: str = "hydro/sensors"
    mqtt_topic_status: str = "hydro/status"
    mqtt_topic_pump_control: str = "hydro/control/pump"
    mqtt_topic_schedule_config: str = "hydro/config/schedule"
    mqtt_topic_sensor_config: str = "hydro/config/sensor"
    mqtt_topic_timezone_config: str = "hydro/config/timezone"
    
    # Timezone (IANA timezone name, e.g., "Australia/Sydney")
    timezone: str = "UTC"
    
    # Database
    database_url: str = "sqlite:///./hydro.db"
    
    # CORS - allow dashboard access
    cors_origins: list[str] = ["http://localhost:5173", "http://localhost:3000"]
    
    class Config:
        env_file = ".env"
        env_prefix = "HYDRO_"


@lru_cache()
def get_settings() -> Settings:
    return Settings()
