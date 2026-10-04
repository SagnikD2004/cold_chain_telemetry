const pool = require("../config/db");

// Register a new ESP32 device
const registerDevice = async (req, res) => {
  try {
    const { node_id, device_name, firmware_version } = req.body;

    if (!node_id || !device_name) {
      return res.status(400).json({
        success: false,
        message: "node_id and device_name are required",
      });
    }

    const result = await pool.query(
      `INSERT INTO devices (node_id, device_name, firmware_version)
       VALUES ($1, $2, $3)
       RETURNING *`,
      [node_id, device_name, firmware_version || null],
    );

    res.status(201).json({
      success: true,
      message: "Device registered successfully",
      device: result.rows[0],
    });
  } catch (error) {
    if (error.code === "23505") {
      return res.status(409).json({
        success: false,
        message: "Device with this node_id already exists",
      });
    }

    console.error("Device registration error:", error);

    res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get all devices with telemetry-based status
const getDevices = async (req, res) => {
  try {
    const result = await pool.query(`
      SELECT
        d.id,
        d.node_id,
        d.device_name,
        d.firmware_version,
        COALESCE(
          latest.battery_pct,
          d.battery_pct
        ) AS battery_pct,
        latest.last_seen_at,
        CASE
          WHEN latest.last_seen_at >= NOW() - INTERVAL '15 seconds'
          THEN 'ONLINE'
          ELSE 'OFFLINE'
        END AS status,
        d.created_at
      FROM devices d
      LEFT JOIN LATERAL (
        SELECT
          MAX(received_at) AS last_seen_at,
          (
            SELECT t2.battery_pct
            FROM telemetry_logs t2
            WHERE t2.node_id = d.node_id
              AND t2.battery_pct IS NOT NULL
            ORDER BY t2.received_at DESC
            LIMIT 1
          ) AS battery_pct
        FROM telemetry_logs t
        WHERE t.node_id = d.node_id
      ) latest ON TRUE
      ORDER BY d.created_at DESC
    `);

    res.json({
      success: true,
      count: result.rowCount,
      devices: result.rows,
    });
  } catch (error) {
    console.error("Fetch devices error:", error);

    res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get device by ID
const getDeviceById = async (req, res) => {
  try {
    const { id } = req.params;

    if (!/^[1-9]\d*$/.test(id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid device ID",
      });
    }

    const result = await pool.query("SELECT * FROM devices WHERE id = $1", [
      id,
    ]);

    if (result.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "Device not found",
      });
    }

    res.json({
      success: true,
      device: result.rows[0],
    });
  } catch (error) {
    console.error("Fetch device error:", error);

    res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

module.exports = {
  registerDevice,
  getDevices,
  getDeviceById,
};
