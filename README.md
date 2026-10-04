# CargoShield

### IoT-Based Intelligent Cold Chain Monitoring and Anomaly Detection System

**CargoShield** is an IoT-based cold-chain shipment monitoring prototype designed to collect sensor telemetry, monitor shipment conditions, detect threshold-based anomalies, and visualize operational data through a real-time web dashboard.

> **Smart Cargo. Secure Conditions.**

## Key Features

- Real-time shipment monitoring dashboard
- Device registration and device selection
- Temperature and vibration monitoring
- Telemetry ingestion and historical readings
- Threshold-based incident detection
- Real-time updates using Socket.IO
- Automatic device online/offline status detection
- Device last-seen tracking
- Trip and device association
- Dashboard summaries and incident visibility
- Telemetry simulation for development and demonstration
- PostgreSQL database integration

## Technology Stack

### Frontend

- React
- Vite
- React Router
- Axios
- Socket.IO Client
- Recharts
- Lucide React

### Backend

- Node.js
- Express.js
- PostgreSQL
- Socket.IO
- JSON Web Tokens
- Zod

### Development Tools

- npm
- Nodemon
- Concurrently
- Git

## Project Structure

```text
Major-project/
│
├── package.json              # Root scripts and project management
├── package-lock.json
│
├── client/                   # React + Vite frontend
│   ├── public/
│   ├── src/
│   │   ├── components/
│   │   ├── pages/
│   │   ├── services/
│   │   └── ...
│   ├── .env
│   └── package.json
│
├── server/                   # Express backend
│   ├── src/
│   │   ├── config/
│   │   ├── controllers/
│   │   ├── routes/
│   │   ├── services/
│   │   └── server.js
│   ├── scripts/
│   │   └── simulateTelemetry.js
│   ├── .env
│   └── package.json
│
└── README.md
```

## Prerequisites

Ensure the following are installed:

- Node.js (compatible with the installed Vite version)
- npm
- PostgreSQL database or a PostgreSQL-compatible hosted database
- Git

Verify Node.js and npm:

```bash
node -v
npm -v
```

## Installation and Initialization

### 1. Clone the repository

```bash
git clone <YOUR_GITHUB_REPOSITORY_URL>
cd Major-project
```

### 2. Install all dependencies

From the project root, execute:

```bash
npm run setup
```

This command installs dependencies for:

- Root project
- Backend
- Frontend

The root `package.json` manages the frontend and backend scripts.

### 3. Configure environment variables

#### Backend configuration

Create `server/.env`:

```env
PORT=8000
DATABASE_URL=your_postgresql_connection_string
CLIENT_URL=http://localhost:5173,http://localhost:4173
```

Replace the database placeholder with your actual PostgreSQL connection string.

For hosted PostgreSQL services, use the provider's appropriate connection URL and SSL configuration.

Do not commit database credentials, JWT secrets, or other sensitive environment variables.

#### Frontend configuration

Create `client/.env`:

```env
VITE_API_URL=http://localhost:8000
```

The frontend uses this URL to communicate with the backend API.

## Running the Application

### Start frontend and backend together

From the project root:

```bash
npm run dev
```

This starts both services concurrently:

| Service          | Local URL                        |
| ---------------- | -------------------------------- |
| React frontend   | http://localhost:5173            |
| Express backend  | http://localhost:8000            |
| API health check | http://localhost:8000/api/health |

Press `Ctrl + C` to stop the development processes.

### Start services individually

Backend:

```bash
npm run server
```

Frontend:

```bash
npm run client
```

### Start the backend in production mode

```bash
npm start
```

## Telemetry Simulator

CargoShield includes a telemetry simulator for testing dashboard functionality without requiring a physical ESP32 device.

Start the simulator from the project root:

```bash
npm run simulator
```

The simulator generates temperature and vibration readings for the configured device and trip.

Current prototype behavior:

- Device ID: `ESP32-CC-001`
- Trip ID: `1`
- Reading interval: approximately 3 seconds
- Temperature and vibration generation
- Threshold-based incident generation

The simulator writes telemetry to the configured PostgreSQL database.

Press `Ctrl + C` to stop it.

**Note:** Simulator-generated telemetry is demonstration data and should not be represented as actual physical sensor measurements.

## Device Monitoring

CargoShield retrieves registered devices from PostgreSQL and allows users to select a device for dashboard monitoring.

### Device status logic

The current prototype uses a 15-second telemetry freshness threshold.

| Condition                                        | Device status |
| ------------------------------------------------ | ------------- |
| Telemetry received within the last 15 seconds    | ONLINE        |
| No telemetry received within the last 15 seconds | OFFLINE       |

The frontend polls the device list every 5 seconds to detect status changes without requiring a browser refresh.

Socket.IO is used to refresh dashboard telemetry and incident information when new events arrive.

Device online status is based on telemetry receipt timestamps. It is not an independent hardware connectivity verification.

## API Overview

Base URL:

```text
http://localhost:8000
```

| Method | Endpoint                         | Purpose                        |
| ------ | -------------------------------- | ------------------------------ |
| GET    | `/api/health`                    | API and database health        |
| GET    | `/api/devices`                   | List registered devices        |
| GET    | `/api/devices/:id`               | Retrieve device details        |
| POST   | `/api/devices`                   | Register a device              |
| GET    | `/api/trips`                     | List trips                     |
| POST   | `/api/trips`                     | Create a trip                  |
| GET    | `/api/telemetry`                 | Retrieve telemetry history     |
| POST   | `/api/telemetry`                 | Submit a telemetry reading     |
| GET    | `/api/telemetry/latest/:node_id` | Retrieve latest device reading |
| GET    | `/api/incidents`                 | List incidents                 |
| GET    | `/api/dashboard/summary`         | Retrieve dashboard summary     |

Some list and dashboard endpoints support device-specific filtering.

For complete request validation, response formats, and available routes, refer to the backend route and controller files.

## Telemetry Request Example

```json
{
  "node_id": "ESP32-CC-001",
  "trip_id": 1,
  "temperature_c": 5.4,
  "vibration_rms": 0.8,
  "recorded_at": "2026-10-04T10:30:00.000Z",
  "seq_counter": 1
}
```

The current telemetry ingestion implementation requires:

- `node_id`
- `temperature_c`
- `vibration_rms`

Other fields may be optional according to the current controller validation.

## Real-Time Communication

CargoShield uses Socket.IO for event-driven dashboard updates.

### Events

| Event           | Description                                         |
| --------------- | --------------------------------------------------- |
| `telemetry:new` | A new telemetry record has been received and stored |
| `incident:new`  | A new incident has been generated                   |

The frontend listens for these events and refreshes relevant dashboard information.

## Database

CargoShield uses PostgreSQL for persistent storage.

The database includes tables for:

- Devices
- Trips
- Telemetry logs
- Incidents
- Users
- Vehicles
- Audit logs

The backend connects to the configured PostgreSQL database through its database configuration module.

## Build and Production

### Build frontend

From the project root:

```bash
npm run build
```

The production frontend output is generated in:

```text
client/dist/
```

### Preview production build

```bash
npm run preview
```

### Run frontend linting

```bash
npm run lint
```

## Available Root Commands

| Command             | Description                                      |
| ------------------- | ------------------------------------------------ |
| `npm run setup`     | Install root, frontend, and backend dependencies |
| `npm run dev`       | Start frontend and backend together              |
| `npm run server`    | Start backend in development mode                |
| `npm run client`    | Start frontend development server                |
| `npm start`         | Start backend in production mode                 |
| `npm run build`     | Build frontend for production                    |
| `npm run preview`   | Preview production frontend                      |
| `npm run lint`      | Run frontend linting                             |
| `npm run simulator` | Start telemetry simulator                        |

## Prototype Scope and Limitations

CargoShield is currently a development and academic prototype.

The current implementation supports temperature and vibration telemetry with threshold-based incident evaluation.

The following capabilities are not yet fully implemented or validated as production features:

- Physical sensor integration and hardware verification
- Complete humidity, MKT, dew point, and battery telemetry
- Validated machine-learning anomaly detection
- Regulatory cold-chain compliance certification
- Production-grade device authentication and deployment security

Device status reflects recent telemetry reception rather than a direct hardware heartbeat.

## Security Considerations

- Keep `.env` files out of version control.
- Use placeholder values in `.env.example`.
- Never expose database credentials or signing secrets.
- Authenticate and authorize device telemetry submissions before deployment.
- Use HTTPS for production communication.
- Apply appropriate database access controls.
- Validate incoming telemetry and API requests.

## Future Enhancements

- Integrate physical ESP32 and sensor hardware
- Implement authenticated device communication
- Add battery and additional environmental measurements
- Introduce configurable threshold policies
- Implement and validate machine-learning anomaly detection
- Add notification and alert management
- Improve historical analytics and reporting
- Implement automated backend and frontend tests
- Deploy the application using a production infrastructure setup

## License

No license has been specified yet. Add a `LICENSE` file before publishing the repository for reuse by others.
