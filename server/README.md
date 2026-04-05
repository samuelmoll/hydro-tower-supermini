# Hydroponic Tower Backend Server

FastAPI backend for the ESP32-C3 hydroponic tower system.

## Features

- MQTT bridge for ESP32 communication
- REST API for web dashboard
- SQLite database for sensor data storage
- Real-time WebSocket updates (optional)

## Quick Start

```bash
# Create virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Start the server
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

## Configuration

Edit `app/config.py` for MQTT broker settings and other configuration.

## API Endpoints

- `GET /api/sensors` - Get sensor readings (with date range filter)
- `GET /api/sensors/latest` - Get most recent sensor reading
- `GET /api/schedule` - Get current pump schedule
- `PUT /api/schedule` - Update pump schedule
- `POST /api/pump/control` - Manual pump control (on/off/auto)
- `GET /api/status` - Get ESP32 device status

## Docker

```bash
docker-compose up -d
```

This starts both Mosquitto MQTT broker and the backend server.
