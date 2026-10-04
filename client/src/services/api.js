import axios from "axios";

const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || "http://localhost:8000",
  headers: { "Content-Type": "application/json" },
});

export const getDevices = async () => {
  const response = await api.get("/api/devices");
  return response.data;
};

export const getDashboardSummary = async (deviceId) => {
  const response = await api.get("/api/dashboard/summary", {
    params: deviceId ? { device_id: deviceId } : {},
  });

  return response.data;
};

export const getTrips = async (deviceId) => {
  const response = await api.get("/api/trips", {
    params: deviceId ? { device_id: deviceId } : {},
  });

  return response.data;
};

export const getTelemetry = async (limit = 20, nodeId) => {
  const response = await api.get("/api/telemetry", {
    params: {
      limit,
      ...(nodeId ? { node_id: nodeId } : {}),
    },
  });

  return response.data;
};

export const getIncidents = async (limit = 20, tripId) => {
  const response = await api.get("/api/incidents", {
    params: {
      limit,
      ...(tripId ? { trip_id: tripId } : {}),
    },
  });

  return response.data;
};

export default api;
