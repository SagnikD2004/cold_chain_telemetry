
CREATE TABLE IF NOT EXISTS users (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role VARCHAR(30) NOT NULL DEFAULT 'OPERATOR'
        CHECK (role IN ('ADMIN', 'OPERATOR', 'QUALITY_MANAGER')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS devices (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    node_id VARCHAR(100) UNIQUE NOT NULL,
    device_name VARCHAR(100) NOT NULL,
    firmware_version VARCHAR(30),
    battery_pct SMALLINT CHECK (battery_pct BETWEEN 0 AND 100),
    last_seen_at TIMESTAMPTZ,
    status VARCHAR(20) NOT NULL DEFAULT 'OFFLINE'
        CHECK (status IN ('ONLINE', 'OFFLINE', 'MAINTENANCE')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS vehicles (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    vehicle_number VARCHAR(30) UNIQUE NOT NULL,
    vehicle_type VARCHAR(50),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS trips (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    trip_code VARCHAR(50) UNIQUE NOT NULL,
    shipment_name VARCHAR(150) NOT NULL,
    device_id BIGINT REFERENCES devices(id),
    vehicle_id BIGINT REFERENCES vehicles(id),
    origin VARCHAR(150) NOT NULL,
    destination VARCHAR(150) NOT NULL,
    start_time TIMESTAMPTZ,
    end_time TIMESTAMPTZ,
    lifecycle_status VARCHAR(20) NOT NULL DEFAULT 'PLANNED'
        CHECK (lifecycle_status IN ('PLANNED', 'IN_PROGRESS', 'COMPLETED', 'CANCELLED')),
    compliance_status VARCHAR(30) NOT NULL DEFAULT 'COMPLIANT'
        CHECK (compliance_status IN ('COMPLIANT', 'EXCURSION_COMPROMISED', 'UNDER_REVIEW')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS telemetry_logs (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    node_id VARCHAR(100) NOT NULL REFERENCES devices(node_id),
    trip_id BIGINT REFERENCES trips(id),
    recorded_at TIMESTAMPTZ NOT NULL,

    temperature_c NUMERIC(6,2) NOT NULL,
    humidity_pct NUMERIC(6,2),
    mkt_c NUMERIC(6,2),
    dew_point_c NUMERIC(6,2),
    vibration_rms NUMERIC(8,4),
    battery_pct SMALLINT CHECK (battery_pct BETWEEN 0 AND 100),

    ml_mse NUMERIC(12,8),
    ml_anomaly_flag BOOLEAN NOT NULL DEFAULT FALSE,
    alert_flags SMALLINT NOT NULL DEFAULT 0,
    seq_counter SMALLINT NOT NULL,

    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),

    UNIQUE (node_id, trip_id, recorded_at)
);

CREATE TABLE IF NOT EXISTS incidents (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    trip_id BIGINT NOT NULL REFERENCES trips(id),
    telemetry_id BIGINT REFERENCES telemetry_logs(id),
    incident_type VARCHAR(40) NOT NULL,
    severity VARCHAR(20) NOT NULL
        CHECK (severity IN ('LOW', 'MEDIUM', 'HIGH', 'CRITICAL')),
    description TEXT NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'OPEN'
        CHECK (status IN ('OPEN', 'ACKNOWLEDGED', 'INVESTIGATING', 'RESOLVED')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    resolved_at TIMESTAMPTZ
);

CREATE TABLE IF NOT EXISTS audit_logs (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    user_id BIGINT REFERENCES users(id),
    action VARCHAR(100) NOT NULL,
    entity_type VARCHAR(50),
    entity_id BIGINT,
    details JSONB,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_telemetry_trip_time
ON telemetry_logs(trip_id, recorded_at DESC);

CREATE INDEX IF NOT EXISTS idx_telemetry_node_time
ON telemetry_logs(node_id, recorded_at DESC);

CREATE INDEX IF NOT EXISTS idx_incidents_trip_status
ON incidents(trip_id, status);

CREATE INDEX IF NOT EXISTS idx_trips_lifecycle
ON trips(lifecycle_status);
