import { useCallback, useEffect, useState } from "react";

import {
  Activity,
  AlertTriangle,
  CheckCircle2,
  Clock3,
  Package,
  RefreshCw,
  Thermometer,
  Wifi,
  WifiOff,
} from "lucide-react";

import {
  getDashboardSummary,
  getTelemetry,
  getTrips,
  getDevices,
} from "../services/api";

import { socket } from "../services/socket";

import {
  Area,
  AreaChart,
  CartesianGrid,
  ReferenceLine,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

// Format timestamps safely
const formatTime = (value) => {
  if (!value) return "--";

  const date = new Date(value);

  if (Number.isNaN(date.getTime())) return "--";

  return date.toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
  });
};

// Reusable statistic card
function StatCard({ title, value, subtitle, icon: Icon, variant = "" }) {
  return (
    <div className="stat-card">
      <div className={`stat-icon ${variant}`}>
        <Icon size={21} />
      </div>

      <div className="stat-content">
        <p>{title}</p>
        <h2>{value ?? 0}</h2>
        <span>{subtitle}</span>
      </div>
    </div>
  );
}

function Dashboard() {
  const [devices, setDevices] = useState([]);
  const [selectedDeviceId, setSelectedDeviceId] = useState("");

  const [summary, setSummary] = useState(null);
  const [telemetry, setTelemetry] = useState([]);
  const [tripRecords, setTripRecords] = useState([]);

  const [connected, setConnected] = useState(socket.connected);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  const loadDevices = useCallback(async () => {
    try {
      const response = await getDevices();

      const deviceList = response.devices || [];

      setDevices(deviceList);

      setSelectedDeviceId((currentId) => {
        if (deviceList.some((device) => device.id === currentId)) {
          return currentId;
        }

        return deviceList[0]?.id || "";
      });
    } catch (err) {
      console.error("Device loading error:", err);
      setError("Unable to load registered devices.");
    }
  }, []);
  // Initial device fetch and periodic status refresh
  useEffect(() => {
    loadDevices();

    const deviceStatusInterval = setInterval(() => {
      loadDevices();
    }, 5000);

    return () => clearInterval(deviceStatusInterval);
  }, [loadDevices]);

  // Fetch dashboard data
  const loadDashboard = useCallback(async () => {
    if (!selectedDeviceId) {
      setSummary(null);
      setTelemetry([]);
      setTripRecords([]);
      setLoading(false);
      return;
    }

    try {
      setError("");

      const selectedDevice = devices.find(
        (device) => device.id === selectedDeviceId,
      );

      if (!selectedDevice) return;

      const [summaryData, telemetryData, tripsData] = await Promise.all([
        getDashboardSummary(selectedDeviceId),
        getTelemetry(20, selectedDevice.node_id),
        getTrips(selectedDeviceId),
      ]);

      setSummary(summaryData);

      setTripRecords(tripsData.trips || []);

      setTelemetry((telemetryData.telemetry || []).slice().reverse());
    } catch (err) {
      console.error("Dashboard loading error:", err);

      setError(
        err.response?.data?.message ||
          "Unable to load dashboard data. Check the backend connection.",
      );
    } finally {
      setLoading(false);
    }
  }, [selectedDeviceId, devices]);

  // Initial load and real-time Socket.IO listeners
  // Maintain Socket.IO connection
  useEffect(() => {
    const handleConnect = () => {
      setConnected(true);
    };

    const handleDisconnect = () => {
      setConnected(false);
    };

    socket.on("connect", handleConnect);
    socket.on("disconnect", handleDisconnect);

    if (!socket.connected) {
      socket.connect();
    }

    return () => {
      socket.off("connect", handleConnect);
      socket.off("disconnect", handleDisconnect);
    };
  }, []);

  // Listen for updates belonging to the selected device
  useEffect(() => {
    if (!selectedDeviceId || devices.length === 0) {
      return;
    }

    const selectedDevice = devices.find(
      (device) => device.id === selectedDeviceId,
    );

    if (!selectedDevice) {
      return;
    }

    const handleNewTelemetry = (data) => {
      const incoming = data?.telemetry;

      if (!incoming || incoming.node_id !== selectedDevice.node_id) {
        return;
      }

      loadDashboard();
      loadDevices();
    };

    const handleNewIncident = () => {
      loadDashboard();
    };

    socket.on("telemetry:new", handleNewTelemetry);
    socket.on("incident:new", handleNewIncident);

    return () => {
      socket.off("telemetry:new", handleNewTelemetry);
      socket.off("incident:new", handleNewIncident);
    };
  }, [selectedDeviceId, devices, loadDashboard]);

  // Fetch dashboard whenever the selected device changes
  useEffect(() => {
    loadDashboard();
  }, [loadDashboard]);

  // Dashboard summary data
  const trips = summary?.trips || {};
  const incidents = summary?.incidents || {};

  const recentIncidents = summary?.recent_incidents || [];

  // Prepare chart data without converting missing values to zero
  const chartData = telemetry
    .filter((item) => item.temperature_c != null)
    .map((item) => ({
      time: formatTime(item.recorded_at),
      temperature: Number(item.temperature_c),
    }));

  const latest = telemetry[telemetry.length - 1];

  // Loading state
  if (loading) {
    return (
      <div className="loading-screen">
        <RefreshCw className="spin" size={24} />
        <p>Loading cold-chain dashboard...</p>
      </div>
    );
  }

  const selectedDevice = devices.find(
    (device) => device.id === selectedDeviceId,
  );
  return (
    <main className="dashboard">
      {/* Header */}

      <header className="topbar">
        <div>
          <p className="eyebrow">PHARMACEUTICAL LOGISTICS</p>

          <h1>Cold Chain Overview</h1>

          <p className="page-description">
            Monitor shipment conditions and operational alerts.
          </p>
        </div>

        <div className="topbar-actions">
          <div className="device-picker">
            <label htmlFor="device-select">Select Device</label>

            <select
              id="device-select"
              value={selectedDeviceId}
              disabled={devices.length === 0}
              onChange={(event) => {
                setLoading(true);
                setSelectedDeviceId(event.target.value);
              }}
            >
              {devices.length === 0 ? (
                <option value="">No registered devices</option>
              ) : (
                devices.map((device) => (
                  <option key={device.id} value={device.id}>
                    {device.device_name} ({device.node_id})
                  </option>
                ))
              )}
            </select>
          </div>

          <div
            className={`connection-pill ${
              selectedDevice?.status === "ONLINE" ? "online" : "offline"
            }`}
          >
            {selectedDevice?.status === "ONLINE" ? (
              <Wifi size={16} />
            ) : (
              <WifiOff size={16} />
            )}

            {selectedDevice?.status || "UNKNOWN"}
          </div>
        </div>
      </header>

      {/* Error message */}
      {error && (
        <div className="error-banner">
          <AlertTriangle size={18} />

          {error}

          <button onClick={loadDashboard}>Retry</button>
        </div>
      )}

      {/* Shipment statistics */}
      <section className="stats-grid">
        <StatCard
          title="Total Shipments"
          value={trips.total_trips}
          subtitle="All registered trips"
          icon={Package}
          variant="blue"
        />

        <StatCard
          title="Active Shipments"
          value={trips.active_trips}
          subtitle="Currently in transit"
          icon={Activity}
          variant="green"
        />

        <StatCard
          title="Completed"
          value={trips.completed_trips}
          subtitle="Completed trips"
          icon={CheckCircle2}
          variant="purple"
        />

        <StatCard
          title="Compromised"
          value={trips.compromised_trips}
          subtitle="Trips requiring review"
          icon={AlertTriangle}
          variant="red"
        />
      </section>

      {/* Live monitoring */}
      <section className="monitor-grid">
        <div className="panel telemetry-panel">
          <div className="panel-heading">
            <div>
              <p className="eyebrow">SENSOR MONITORING</p>

              <h2>Live Telemetry</h2>
            </div>

            <span className="live-indicator">
              <span />
              LIVE
            </span>
          </div>

          <div className="reading-grid">
            {/* Temperature */}
            <div className="reading-card">
              <div className="reading-label">
                <Thermometer size={18} />
                Temperature
              </div>

              <h3>
                {latest?.temperature_c != null
                  ? `${Number(latest.temperature_c).toFixed(1)}°C`
                  : "--"}
              </h3>

              <span className="reading-range">Prototype range: 2–8°C</span>
            </div>

            {/* Vibration */}
            <div className="reading-card">
              <div className="reading-label">
                <Activity size={18} />
                Vibration
              </div>

              <h3>
                {latest?.vibration_rms != null
                  ? `${Number(latest.vibration_rms).toFixed(2)}g`
                  : "--"}
              </h3>

              <span className="reading-range">Prototype threshold: 2.5g</span>
            </div>
          </div>

          <div className="reading-footer">
            <Clock3 size={15} />
            Last reading: {formatTime(latest?.recorded_at)}
          </div>
        </div>

        {/* Incident summary */}
        <div className="panel incident-summary">
          <div className="panel-heading">
            <div>
              <p className="eyebrow">ALERT MANAGEMENT</p>

              <h2>Incident Summary</h2>
            </div>

            <AlertTriangle size={21} />
          </div>

          <div className="incident-total">
            <strong>{incidents.active_incidents ?? 0}</strong>

            <span>Active incidents</span>
          </div>

          <div className="incident-breakdown">
            <div>
              <span className="severity-dot critical" />
              Critical
              <strong>{incidents.critical_incidents ?? 0}</strong>
            </div>

            <div>
              <span className="severity-dot high" />
              High
              <strong>{incidents.high_incidents ?? 0}</strong>
            </div>

            <div>
              <span className="severity-dot resolved" />
              Resolved
              <strong>{incidents.resolved_incidents ?? 0}</strong>
            </div>
          </div>
        </div>
      </section>

      {/* Temperature history */}
      <section className="panel chart-panel">
        <div className="panel-heading">
          <div>
            <p className="eyebrow">ENVIRONMENTAL HISTORY</p>

            <h2>Temperature Trend</h2>
          </div>

          <span className="chart-unit">°C</span>
        </div>

        <div className="chart-container">
          {chartData.length > 0 ? (
            <ResponsiveContainer width="100%" height="100%">
              <AreaChart data={chartData}>
                <defs>
                  <linearGradient
                    id="temperatureGradient"
                    x1="0"
                    y1="0"
                    x2="0"
                    y2="1"
                  >
                    <stop offset="0%" stopColor="#3b82f6" stopOpacity={0.25} />

                    <stop offset="95%" stopColor="#3b82f6" stopOpacity={0} />
                  </linearGradient>
                </defs>

                <CartesianGrid strokeDasharray="3 3" stroke="#e8edf4" />

                <XAxis
                  dataKey="time"
                  tick={{ fontSize: 12 }}
                  stroke="#94a3b8"
                />

                <YAxis
                  domain={["auto", "auto"]}
                  tick={{ fontSize: 12 }}
                  stroke="#94a3b8"
                />

                <Tooltip />

                {/* Prototype temperature limits */}
                <ReferenceLine
                  y={2}
                  stroke="#16a34a"
                  strokeDasharray="5 5"
                  label={{
                    value: "Minimum 2°C",
                    position: "insideTopLeft",
                    fill: "#16a34a",
                    fontSize: 10,
                  }}
                />

                <ReferenceLine
                  y={8}
                  stroke="#dc2626"
                  strokeDasharray="5 5"
                  label={{
                    value: "Maximum 8°C",
                    position: "insideTopLeft",
                    fill: "#dc2626",
                    fontSize: 10,
                  }}
                />

                <Area
                  type="monotone"
                  dataKey="temperature"
                  name="Temperature"
                  stroke="#3b82f6"
                  strokeWidth={2.5}
                  fill="url(#temperatureGradient)"
                  connectNulls={false}
                />
              </AreaChart>
            </ResponsiveContainer>
          ) : (
            <div className="empty-state">No telemetry readings available.</div>
          )}
        </div>
      </section>

      {/* Recent incidents */}
      <section className="panel incidents-panel">
        <div className="panel-heading">
          <div>
            <p className="eyebrow">RECENT EVENTS</p>

            <h2>Recent Incidents</h2>
          </div>

          <span className="table-count">{recentIncidents.length} records</span>
        </div>

        <div className="table-wrapper">
          <table>
            <thead>
              <tr>
                <th>Incident</th>
                <th>Shipment</th>
                <th>Severity</th>
                <th>Status</th>
                <th>Time</th>
              </tr>
            </thead>

            <tbody>
              {recentIncidents.map((incident) => (
                <tr key={incident.id}>
                  <td>
                    <strong>{incident.incident_type}</strong>

                    <span className="table-description">
                      {incident.description}
                    </span>
                  </td>

                  <td>{incident.trip_code || `Trip #${incident.trip_id}`}</td>

                  <td>
                    <span
                      className={`severity-badge ${
                        incident.severity?.toLowerCase() || ""
                      }`}
                    >
                      {incident.severity || "N/A"}
                    </span>
                  </td>

                  <td>
                    <span className="status-text">
                      {incident.status || "N/A"}
                    </span>
                  </td>

                  <td>{formatTime(incident.created_at)}</td>
                </tr>
              ))}

              {recentIncidents.length === 0 && (
                <tr>
                  <td colSpan="5" className="empty-state">
                    No incidents found.
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </section>

      {/* Shipment details */}
      <section className="panel shipments-panel">
        <div className="panel-heading">
          <div>
            <p className="eyebrow">SHIPMENT MANAGEMENT</p>

            <h2>Shipment Details</h2>
          </div>

          <Package size={21} />
        </div>

        {tripRecords.length > 0 ? (
          <div className="shipment-list">
            {tripRecords.map((trip) => {
              const complianceClass =
                trip.compliance_status === "COMPLIANT"
                  ? "compliance-ok"
                  : trip.compliance_status === "EXCURSION_COMPROMISED"
                    ? "compliance-warning"
                    : "compliance-neutral";

              return (
                <div className="shipment-card" key={trip.id}>
                  <div className="shipment-header">
                    <div>
                      <span className="shipment-code">{trip.trip_code}</span>

                      <h3>{trip.shipment_name}</h3>
                    </div>

                    <span
                      className={`shipment-status ${
                        trip.lifecycle_status === "COMPLETED"
                          ? "completed"
                          : "active"
                      }`}
                    >
                      {trip.lifecycle_status || "UNKNOWN"}
                    </span>
                  </div>

                  <div className="shipment-route">
                    <div>
                      <span>ORIGIN</span>
                      <strong>{trip.origin || "N/A"}</strong>
                    </div>

                    <div className="route-line">→</div>

                    <div>
                      <span>DESTINATION</span>
                      <strong>{trip.destination || "N/A"}</strong>
                    </div>
                  </div>

                  <div className="shipment-meta">
                    <div>
                      <span>Assigned Device</span>

                      <strong>{trip.node_id || "N/A"}</strong>
                    </div>

                    <div>
                      <span>Compliance</span>

                      <strong className={complianceClass}>
                        {trip.compliance_status || "N/A"}
                      </strong>
                    </div>
                  </div>
                </div>
              );
            })}
          </div>
        ) : (
          <div className="empty-state">No shipment records found.</div>
        )}
      </section>

      {/* Footer */}
      <footer className="dashboard-footer">
        <span>Cold Chain Monitoring System</span>
        <span>Rule-based prototype monitoring</span>
      </footer>
    </main>
  );
}

export default Dashboard;
