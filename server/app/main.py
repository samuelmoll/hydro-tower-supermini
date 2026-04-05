"""
Hydroponic Tower Backend - FastAPI Application

Main entry point for the server application.
"""

import logging
from contextlib import asynccontextmanager
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .config import get_settings
from .database import create_tables
from .mqtt_client import get_mqtt_client
from .api import sensors, schedule, control

# Configure logging
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)

settings = get_settings()


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan manager"""
    # Startup
    logger.info("Starting Hydroponic Tower Backend...")
    
    # Create database tables
    create_tables()
    logger.info("Database tables created")
    
    # Connect to MQTT broker
    mqtt_client = get_mqtt_client()
    try:
        mqtt_client.connect()
        logger.info("MQTT client connected")
    except Exception as e:
        logger.warning(f"MQTT connection failed: {e}. Will retry in background.")
    
    yield
    
    # Shutdown
    logger.info("Shutting down...")
    mqtt_client.disconnect()


# Create FastAPI application
app = FastAPI(
    title=settings.app_name,
    description="Backend API for ESP32-C3 Hydroponic Tower Controller",
    version="1.0.0",
    lifespan=lifespan
)

# Configure CORS for dashboard access
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include API routers
app.include_router(sensors.router, prefix="/api")
app.include_router(schedule.router, prefix="/api")
app.include_router(control.router, prefix="/api")


@app.get("/")
def root():
    """Root endpoint - API health check"""
    return {
        "name": settings.app_name,
        "status": "running",
        "mqtt_connected": get_mqtt_client().connected
    }


@app.get("/health")
def health_check():
    """Health check endpoint"""
    mqtt_client = get_mqtt_client()
    return {
        "status": "healthy",
        "mqtt_connected": mqtt_client.connected
    }
