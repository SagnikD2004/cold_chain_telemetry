const pool = require("../config/db");

const getDashboardSummary = async (req, res) => {
  try {
    const { device_id } = req.query;

    if (device_id !== undefined && !/^[1-9]\d*$/.test(device_id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid device ID",
      });
    }

    const deviceFilter = device_id !== undefined;
    const deviceId = deviceFilter ? device_id : null;

    const [
      tripStatsResult,
      telemetryStatsResult,
      incidentStatsResult,
      recentIncidentsResult,
    ] = await Promise.all([
      pool.query(
        `
        SELECT
          COUNT(*)::INTEGER AS total_trips,
          COUNT(*) FILTER (
            WHERE lifecycle_status = 'PLANNED'
          )::INTEGER AS planned_trips,
          COUNT(*) FILTER (
            WHERE lifecycle_status = 'IN_PROGRESS'
          )::INTEGER AS active_trips,
          COUNT(*) FILTER (
            WHERE lifecycle_status = 'COMPLETED'
          )::INTEGER AS completed_trips,
          COUNT(*) FILTER (
            WHERE compliance_status = 'EXCURSION_COMPROMISED'
          )::INTEGER AS compromised_trips,
          COUNT(*) FILTER (
            WHERE compliance_status = 'UNDER_REVIEW'
          )::INTEGER AS trips_under_review
        FROM trips
        WHERE ($1::bigint IS NULL OR device_id = $1)
        `,
        [deviceId],
      ),

      pool.query(
        `
        SELECT
          COUNT(*)::INTEGER AS total_readings,
          ROUND(AVG(temperature_c), 2) AS average_temperature,
          ROUND(MIN(temperature_c), 2) AS minimum_temperature,
          ROUND(MAX(temperature_c), 2) AS maximum_temperature,
          MAX(received_at) AS last_reading_received
        FROM telemetry_logs
        WHERE (
          $1::bigint IS NULL
          OR node_id IN (
            SELECT node_id
            FROM devices
            WHERE id = $1
          )
        )
        `,
        [deviceId],
      ),

      pool.query(
        `
        SELECT
          COUNT(*) FILTER (
            WHERE i.status != 'RESOLVED'
          )::INTEGER AS active_incidents,
          COUNT(*) FILTER (
            WHERE i.status != 'RESOLVED'
              AND i.severity = 'CRITICAL'
          )::INTEGER AS critical_incidents,
          COUNT(*) FILTER (
            WHERE i.status != 'RESOLVED'
              AND i.severity = 'HIGH'
          )::INTEGER AS high_incidents,
          COUNT(*) FILTER (
            WHERE i.status = 'RESOLVED'
          )::INTEGER AS resolved_incidents
        FROM incidents i
        JOIN trips t ON t.id = i.trip_id
        WHERE ($1::bigint IS NULL OR t.device_id = $1)
        `,
        [deviceId],
      ),

      pool.query(
        `
        SELECT
          i.id,
          i.trip_id,
          t.trip_code,
          i.telemetry_id,
          i.incident_type,
          i.severity,
          i.description,
          i.status,
          i.created_at
        FROM incidents i
        JOIN trips t ON t.id = i.trip_id
        WHERE ($1::bigint IS NULL OR t.device_id = $1)
        ORDER BY i.created_at DESC
        LIMIT 5
        `,
        [deviceId],
      ),
    ]);

    return res.json({
      success: true,
      message: "Dashboard summary retrieved successfully",
      generated_at: new Date().toISOString(),
      trips: tripStatsResult.rows[0],
      telemetry: telemetryStatsResult.rows[0],
      incidents: incidentStatsResult.rows[0],
      recent_incidents: recentIncidentsResult.rows,
    });
  } catch (error) {
    console.error("Dashboard summary error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

module.exports = {
  getDashboardSummary,
};
