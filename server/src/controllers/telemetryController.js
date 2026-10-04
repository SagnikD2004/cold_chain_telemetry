const pool = require("../config/db");
const { getIO } = require("../config/socket");
const { evaluateTelemetry } = require("../services/alertService");

// Receive a single telemetry reading
const ingestTelemetry = async (req, res) => {
  try {
    const {
      node_id,
      trip_id,
      temperature_c,
      vibration_rms,
      recorded_at,
      seq_counter,
    } = req.body;

    // Validate required fields
    if (
      !node_id ||
      temperature_c === undefined ||
      temperature_c === null ||
      vibration_rms === undefined ||
      vibration_rms === null
    ) {
      return res.status(400).json({
        success: false,
        message: "node_id, temperature_c and vibration_rms are required",
      });
    }

    // Validate numeric readings
    if (
      !Number.isFinite(Number(temperature_c)) ||
      !Number.isFinite(Number(vibration_rms)) ||
      Number(vibration_rms) < 0
    ) {
      return res.status(400).json({
        success: false,
        message: "Invalid temperature or vibration value",
      });
    }

    // Validate timestamp
    const timestamp = recorded_at || new Date().toISOString();

    if (Number.isNaN(Date.parse(timestamp))) {
      return res.status(400).json({
        success: false,
        message: "Invalid recorded_at timestamp",
      });
    }

    // Verify device exists
    const deviceResult = await pool.query(
      "SELECT id FROM devices WHERE node_id = $1",
      [node_id],
    );

    if (deviceResult.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "Device not found",
      });
    }

    // Verify trip if supplied
    if (trip_id !== undefined && trip_id !== null) {
      const tripResult = await pool.query(
        "SELECT id FROM trips WHERE id = $1 AND device_id = $2",
        [trip_id, deviceResult.rows[0].id],
      );

      if (tripResult.rowCount === 0) {
        return res.status(400).json({
          success: false,
          message: "Trip does not exist or is not assigned to this device",
        });
      }
    }

    // Store telemetry
    const result = await pool.query(
      `INSERT INTO telemetry_logs (
        node_id,
        trip_id,
        recorded_at,
        temperature_c,
        humidity_pct,
        mkt_c,
        dew_point_c,
        vibration_rms,
        battery_pct,
        ml_mse,
        ml_anomaly_flag,
        seq_counter
      )
      VALUES (
        $1, $2, $3, $4, NULL, NULL, NULL, $5, NULL, NULL, FALSE, $6
      )
      RETURNING *`,
      [
        node_id,
        trip_id || null,
        timestamp,
        Number(temperature_c),
        Number(vibration_rms),
        seq_counter ?? null,
      ],
    );

    const savedTelemetry = result.rows[0];

    let incidents = [];
    let alertProcessingError = false;
    const io = getIO();

    try {
      incidents = await evaluateTelemetry(savedTelemetry);
    } catch (alertError) {
      alertProcessingError = true;
      console.error("Alert evaluation error:", alertError);
    }

    // Emit real-time events to connected dashboard clients
    try {
      const io = getIO();

      io.emit("telemetry:new", {
        success: true,
        telemetry: savedTelemetry,
      });

      for (const incident of incidents) {
        io.emit("incident:new", {
          success: true,
          incident,
        });
      }
    } catch (socketError) {
      console.error("Socket.IO emission error:", socketError.message);
    }

    return res.status(201).json({
      success: true,
      message: "Telemetry recorded successfully",
      telemetry: savedTelemetry,
      incidents,
      alert_processing: alertProcessingError ? "FAILED" : "COMPLETED",
    });
  } catch (error) {
    console.error("Telemetry ingestion error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get telemetry history
const getTelemetryHistory = async (req, res) => {
  try {
    const { node_id, trip_id, limit = 100 } = req.query;

    const parsedLimit = Number(limit);

    if (
      !Number.isInteger(parsedLimit) ||
      parsedLimit < 1 ||
      parsedLimit > 1000
    ) {
      return res.status(400).json({
        success: false,
        message: "Limit must be an integer between 1 and 1000",
      });
    }

    const conditions = [];
    const values = [];

    if (node_id) {
      values.push(node_id);
      conditions.push(`node_id = $${values.length}`);
    }

    if (trip_id) {
      if (!/^[1-9]\d*$/.test(trip_id)) {
        return res.status(400).json({
          success: false,
          message: "Invalid trip_id",
        });
      }

      values.push(trip_id);
      conditions.push(`trip_id = $${values.length}`);
    }

    values.push(parsedLimit);

    const whereClause =
      conditions.length > 0 ? `WHERE ${conditions.join(" AND ")}` : "";

    const result = await pool.query(
      `SELECT *
       FROM telemetry_logs
       ${whereClause}
       ORDER BY recorded_at DESC
       LIMIT $${values.length}`,
      values,
    );

    return res.json({
      success: true,
      count: result.rowCount,
      telemetry: result.rows,
    });
  } catch (error) {
    console.error("Fetch telemetry history error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get latest telemetry for a device
const getLatestTelemetry = async (req, res) => {
  try {
    const { node_id } = req.params;

    const result = await pool.query(
      `SELECT *
       FROM telemetry_logs
       WHERE node_id = $1
       ORDER BY recorded_at DESC, id DESC
       LIMIT 1`,
      [node_id],
    );

    if (result.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "No telemetry found for this device",
      });
    }

    return res.json({
      success: true,
      telemetry: result.rows[0],
    });
  } catch (error) {
    console.error("Fetch latest telemetry error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

module.exports = {
  ingestTelemetry,
  getTelemetryHistory,
  getLatestTelemetry,
};
