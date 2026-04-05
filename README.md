# ESP32-C3 Hydroponic Tower Controller

Complete IoT system for monitoring and controlling a hydroponic tower with ESP32-C3 microcontroller.

## System Architecture

```
┌─────────────────┐      MQTT       ┌──────────────────┐
│   ESP32-C3      │ ◄─────────────► │  Mosquitto       │
│  (Hydro Tower)  │                 │  MQTT Broker     │
└─────────────────┘                 └────────┬─────────┘
   │ DHT11 sensor                            │
   │ Relay → Pump                            ▼
                                    ┌──────────────────┐
                                    │  Backend (FastAPI)│──► SQLite
                                    └────────┬─────────┘
                                             │ REST API
                                             ▼
                                    ┌──────────────────┐
                                    │   Web Dashboard  │ ← Browser
                                    └──────────────────┘
```

## Features

### ESP32 Firmware
- **DHT11 sensor** - Temperature and humidity monitoring
- **Relay pump control** - Scheduled and manual operation
- **WiFi connectivity** - Auto-reconnection with exponential backoff
- **MQTT communication** - Real-time bidirectional messaging
- **NTP time sync** - Accurate scheduling
- **Persistent storage** - Schedules saved in flash (NVS)

### Backend Server
- **FastAPI** - Modern async Python REST API
- **MQTT bridge** - Subscribes to ESP32 data, publishes commands
- **SQLite database** - Sensor history and configuration
- **Docker support** - Easy deployment with docker-compose

### Web Dashboard
- **Vue 3 + Vite** - Fast, modern SPA
- **Tailwind CSS** - Clean, responsive design
- **Real-time data** - Live sensor readings and pump status
- **Schedule editor** - Visual pump schedule management
- **History charts** - Temperature/humidity graphs with Chart.js

---

## Quick Start

### 1. ESP32 Firmware

1. **Configure credentials** in `src/config.h`:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define MQTT_BROKER "YOUR_SERVER_IP"
   ```

2. **Wire hardware**:
   - DHT11 data → GPIO 4
   - Relay signal → GPIO 5
   - Both need 3.3V/5V power and GND

3. **Upload firmware** via PlatformIO:
   - Open project in VS Code with PlatformIO extension
   - Click Upload (→) button

### 2. Server (Docker)

```bash
cd server

# Generate MQTT password
docker run -it --rm -v $(pwd)/mosquitto/config:/mosquitto/config \
  eclipse-mosquitto:2 mosquitto_passwd -c /mosquitto/config/passwd hydro
# Enter password: hydro_secret

# Start services
docker-compose up -d
```

Services:
- **MQTT Broker**: Port 1883
- **Backend API**: http://localhost:8000
- **Dashboard**: http://localhost:80

### 3. Server (Manual)

```bash
cd server

# Python backend
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000

# Dashboard (separate terminal)
cd ../dashboard
npm install
npm run dev
```

---

## Hardware Setup

### Bill of Materials
| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32-C3 Super Mini | 1 | Or any ESP32-C3 board |
| DHT11 sensor | 1 | Or DHT22 for better precision |
| 5V Relay module | 1 | Single channel, 3.3V logic |
| Water pump | 1 | 5V-12V submersible pump |
| Power supply | 1 | 5V 2A recommended |
| Jumper wires | ~10 | Male-female and male-male |

### Wiring Diagram
```
ESP32-C3           DHT11
─────────────────────────
3.3V ────────────── VCC
GND  ────────────── GND
GPIO 4 ──────────── DATA

ESP32-C3           Relay Module
─────────────────────────────
5V/VIN ──────────── VCC
GND ─────────────── GND
GPIO 5 ──────────── IN

Relay              Pump
─────────────────────────
COM ─────────────── Pump +
NO ──────────────── Power +
                   Pump - ──── Power -
```

---

## API Reference

### Sensors
```
GET /api/sensors              # Get readings (with date filters)
GET /api/sensors/latest       # Latest reading
GET /api/sensors/stats?hours=24  # Statistics
```

### Schedule
```
GET /api/schedule             # Get all schedules
PUT /api/schedule             # Replace all schedules
POST /api/schedule            # Add single schedule
DELETE /api/schedule/{id}     # Delete schedule
```

### Control
```
POST /api/pump/control        # {"action": "on|off|auto", "duration_min": 5}
GET /api/status               # Device status
POST /api/sensor/config?interval_sec=60  # Set sensor interval
```

---

## MQTT Topics

### Published by ESP32
| Topic | Payload | Description |
|-------|---------|-------------|
| `hydro/sensors` | `{"temperature": 25.5, "humidity": 60.0, "timestamp": ...}` | Sensor readings |
| `hydro/status` | `{"pump_state": "on", "wifi_rssi": -55, "uptime_sec": 3600}` | Device status |

### Subscribed by ESP32
| Topic | Payload | Description |
|-------|---------|-------------|
| `hydro/control/pump` | `{"action": "on\|off\|auto"}` | Manual pump control |
| `hydro/config/schedule` | `{"schedules": [{hour, minute, duration_min, enabled}]}` | Schedule update |
| `hydro/config/sensor` | `{"interval_sec": 60}` | Sensor interval |

---

## Troubleshooting

### ESP32 won't connect to WiFi
- Check SSID/password in `config.h`
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check serial monitor for debug messages

### MQTT connection failing
- Verify broker IP is correct
- Check broker is running: `mosquitto_sub -h localhost -t '#'`
- Verify username/password match

### Sensor readings show NaN
- Check DHT11 wiring (data pin, power, ground)
- Try different GPIO pin
- Ensure 2 second delay after power-on

### Dashboard not loading data
- Check backend is running
- Verify CORS settings in `server/app/config.py`
- Check browser console for errors

---

## Project Structure

```
ESP32-C3_HYDRO/
├── src/                      # ESP32 firmware
│   ├── main.cpp             # Main application
│   ├── config.h             # Configuration
│   ├── wifi_manager.cpp/h   # WiFi connection
│   ├── mqtt_handler.cpp/h   # MQTT communication
│   ├── sensors.cpp/h        # DHT11 reader
│   └── pump_controller.cpp/h # Pump control
├── platformio.ini           # PlatformIO config
│
├── server/                   # Backend service
│   ├── app/
│   │   ├── main.py          # FastAPI app
│   │   ├── config.py        # Settings
│   │   ├── database.py      # SQLAlchemy models
│   │   ├── mqtt_client.py   # MQTT bridge
│   │   ├── schemas.py       # Pydantic schemas
│   │   └── api/             # REST endpoints
│   ├── requirements.txt
│   ├── Dockerfile
│   └── docker-compose.yml
│
└── dashboard/                # Vue 3 frontend
    ├── src/
    │   ├── App.vue
    │   ├── main.js
    │   ├── api/client.js    # API client
    │   └── views/           # Page components
    ├── package.json
    └── Dockerfile
```

---

## License

MIT License - Feel free to use for personal and commercial projects.
