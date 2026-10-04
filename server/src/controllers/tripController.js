const { evaluateTripCompliance } = require("../services/complianceService");
const pool = require("../config/db");

// Create a new shipment trip
const createTrip = async (req, res) => {
  try {
    const {
      trip_code,
      shipment_name,
      device_id,
      vehicle_id,
      origin,
      destination,
      start_time,
    } = req.body;

    if (!trip_code || !shipment_name || !device_id || !origin || !destination) {
      return res.status(400).json({
        success: false,
        message:
          "trip_code, shipment_name, device_id, origin and destination are required",
      });
    }

    const result = await pool.query(
      `INSERT INTO trips
       (trip_code, shipment_name, device_id, vehicle_id,
        origin, destination, start_time)
       VALUES ($1, $2, $3, $4, $5, $6, $7)
       RETURNING *`,
      [
        trip_code,
        shipment_name,
        device_id,
        vehicle_id || null,
        origin,
        destination,
        start_time || null,
      ],
    );

    res.status(201).json({
      success: true,
      message: "Trip created successfully",
      trip: result.rows[0],
    });
  } catch (error) {
    if (error.code === "23505") {
      return res.status(409).json({
        success: false,
        message: "Trip code already exists",
      });
    }

    if (error.code === "23503") {
      return res.status(400).json({
        success: false,
        message: "Invalid device_id or vehicle_id",
      });
    }

    console.error("Create trip error:", error);

    res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get all trips
const getTrips = async (req, res) => {
  try {
    const { device_id } = req.query;

    let query = `
      SELECT
        t.*,
        d.node_id,
        d.device_name,
        v.vehicle_number
      FROM trips t
      LEFT JOIN devices d ON t.device_id = d.id
      LEFT JOIN vehicles v ON t.vehicle_id = v.id
    `;

    const values = [];

    if (device_id !== undefined) {
      if (!/^[1-9]\d*$/.test(device_id)) {
        return res.status(400).json({
          success: false,
          message: "Invalid device ID",
        });
      }

      query += ` WHERE t.device_id = $1`;
      values.push(device_id);
    }

    query += ` ORDER BY t.created_at DESC`;

    const result = await pool.query(query, values);

    return res.json({
      success: true,
      count: result.rowCount,
      trips: result.rows,
    });
  } catch (error) {
    console.error("Fetch trips error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get trip by ID
const getTripById = async (req, res) => {
  try {
    const { id } = req.params;

    if (!/^[1-9]\d*$/.test(id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid trip ID",
      });
    }

    const result = await pool.query(
      `SELECT
        t.*,
        d.node_id,
        d.device_name,
        v.vehicle_number
       FROM trips t
       LEFT JOIN devices d ON t.device_id = d.id
       LEFT JOIN vehicles v ON t.vehicle_id = v.id
       WHERE t.id = $1`,
      [id],
    );

    if (result.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "Trip not found",
      });
    }

    res.json({
      success: true,
      trip: result.rows[0],
    });
  } catch (error) {
    console.error("Fetch trip error:", error);

    res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Update trip lifecycle status
const updateTripStatus = async (req, res) => {
  try {
    const { id } = req.params;
    const { lifecycle_status } = req.body;

    const allowedStatuses = [
      "PLANNED",
      "IN_PROGRESS",
      "COMPLETED",
      "CANCELLED",
    ];

    if (!/^[1-9]\d*$/.test(id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid trip ID",
      });
    }

    if (!allowedStatuses.includes(lifecycle_status)) {
      return res.status(400).json({
        success: false,
        message: "Invalid lifecycle status",
      });
    }

    const result = await pool.query(
      `UPDATE trips
   SET lifecycle_status = $1::VARCHAR,
       start_time = CASE
         WHEN $1::VARCHAR = 'IN_PROGRESS'
           THEN COALESCE(start_time, NOW())
         ELSE start_time
       END,
       end_time = CASE
         WHEN $1::VARCHAR IN ('COMPLETED', 'CANCELLED')
           THEN NOW()
         ELSE NULL
       END
   WHERE id = $2
   RETURNING *`,
      [lifecycle_status, id],
    );

    if (result.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "Trip not found",
      });
    }

    res.json({
      success: true,
      message: "Trip status updated successfully",
      trip: result.rows[0],
    });
  } catch (error) {
    console.error("Update trip status error:", error);

    res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get calculated compliance for a trip
const getTripCompliance = async (req, res) => {
  try {
    const { id } = req.params;

    if (!/^[1-9]\d*$/.test(id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid trip ID",
      });
    }

    const result = await evaluateTripCompliance(id);

    return res.json({
      success: true,
      message: "Trip compliance evaluated successfully",
      ...result,
    });
  } catch (error) {
    if (error.statusCode === 404) {
      return res.status(404).json({
        success: false,
        message: error.message,
      });
    }

    console.error("Trip compliance error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

module.exports = {
  createTrip,
  getTrips,
  getTripById,
  updateTripStatus,
  getTripCompliance,
};
