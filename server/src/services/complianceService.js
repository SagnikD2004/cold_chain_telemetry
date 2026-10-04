
const pool = require("../config/db");

const TEMP_MIN = 2;
const TEMP_MAX = 8;

const evaluateTripCompliance = async (tripId) => {
  const tripResult = await pool.query(
    "SELECT id, trip_code FROM trips WHERE id = $1",
    [tripId]
  );

  if (tripResult.rowCount === 0) {
    const error = new Error("Trip not found");
    error.statusCode = 404;
    throw error;
  }

  const telemetryResult = await pool.query(
    `SELECT
       COUNT(*)::INTEGER AS total_readings,
       COUNT(temperature_c)::INTEGER AS valid_temperature_readings,
       COUNT(*) FILTER (
         WHERE temperature_c IS NULL
       )::INTEGER AS missing_temperature_readings,
       COUNT(*) FILTER (
         WHERE temperature_c < $2 OR temperature_c > $3
       )::INTEGER AS excursion_readings,
       MIN(temperature_c) AS min_temperature,
       MAX(temperature_c) AS max_temperature
     FROM telemetry_logs
     WHERE trip_id = $1`,
    [tripId, TEMP_MIN, TEMP_MAX]
  );

  const stats = telemetryResult.rows[0];

  const totalReadings = stats.total_readings;
  const validReadings = stats.valid_temperature_readings;
  const excursionReadings = stats.excursion_readings;

  let complianceStatus;

  if (excursionReadings > 0) {
    complianceStatus = "EXCURSION_COMPROMISED";
  } else if (
    totalReadings === 0 ||
    validReadings !== totalReadings
  ) {
    complianceStatus = "UNDER_REVIEW";
  } else {
    complianceStatus = "COMPLIANT";
  }

  const compliancePercentage =
    validReadings > 0
      ? Number(
          (
            ((validReadings - excursionReadings) / validReadings) *
            100
          ).toFixed(2)
        )
      : null;

  const updateResult = await pool.query(
    `UPDATE trips
     SET compliance_status = $1::VARCHAR
     WHERE id = $2
     RETURNING id, trip_code, lifecycle_status, compliance_status`,
    [complianceStatus, tripId]
  );

  return {
    trip: updateResult.rows[0],
    summary: {
      total_readings: totalReadings,
      valid_temperature_readings: validReadings,
      missing_temperature_readings: stats.missing_temperature_readings,
      excursion_readings: excursionReadings,
      min_temperature: stats.min_temperature,
      max_temperature: stats.max_temperature,
      compliance_percentage: compliancePercentage,
      permitted_temperature_range: {
        min: TEMP_MIN,
        max: TEMP_MAX,
        unit: "°C",
      },
    },
  };
};

module.exports = {
  evaluateTripCompliance,
};
