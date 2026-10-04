
const pool = require("../config/db");

const TEMP_MIN = 2;
const TEMP_MAX = 8;
const VIBRATION_MAX = 2.5;

async function evaluateTelemetry(reading) {
  const {
    id: telemetryId,
    trip_id: tripId,
    temperature_c: temperature,
    vibration_rms: vibration,
  } = reading;

  // Incidents require a trip association.
  if (!tripId || !telemetryId) {
    return [];
  }

  const incidents = [];

  // Temperature rule
  if (temperature !== null && temperature !== undefined) {
    if (temperature < TEMP_MIN || temperature > TEMP_MAX) {
      const extreme = temperature < 0 || temperature > 10;

      incidents.push({
        type: "TEMP_OUT_OF_RANGE",
        severity: extreme ? "CRITICAL" : "HIGH",
        description:
          `Temperature ${temperature}°C is outside the permitted ` +
          `prototype range of ${TEMP_MIN}°C to ${TEMP_MAX}°C.`,
      });
    }
  }

  // Vibration rule
  if (vibration !== null && vibration !== undefined) {
    if (vibration > VIBRATION_MAX) {
      incidents.push({
        type: "EXCESSIVE_VIBRATION",
        severity: "HIGH",
        description:
          `Vibration ${vibration}g exceeded the prototype threshold ` +
          `of ${VIBRATION_MAX}g.`,
      });
    }
  }

  const createdIncidents = [];

  for (const incident of incidents) {
    const result = await pool.query(
      `INSERT INTO incidents
        (trip_id, telemetry_id, incident_type, severity, description)
       VALUES ($1, $2, $3, $4, $5)
       RETURNING *`,
      [
        tripId,
        telemetryId,
        incident.type,
        incident.severity,
        incident.description,
      ]
    );

    createdIncidents.push(result.rows[0]);
  }

  return createdIncidents;
}

module.exports = {
  evaluateTelemetry,
};
