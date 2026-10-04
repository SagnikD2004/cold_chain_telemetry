const pool = require("../config/db");

// Get all incidents with optional filters
const getIncidents = async (req, res) => {
  try {
    const { trip_id, severity, status, limit = 100 } = req.query;

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

    if (trip_id !== undefined) {
      if (!/^[1-9]\d*$/.test(trip_id)) {
        return res.status(400).json({
          success: false,
          message: "Invalid trip_id",
        });
      }

      values.push(trip_id);
      conditions.push(`trip_id = $${values.length}`);
    }

    const allowedSeverities = ["LOW", "MEDIUM", "HIGH", "CRITICAL"];

    if (severity !== undefined) {
      if (!allowedSeverities.includes(severity)) {
        return res.status(400).json({
          success: false,
          message: "Invalid severity",
        });
      }

      values.push(severity);
      conditions.push(`severity = $${values.length}`);
    }

    const allowedStatuses = [
      "OPEN",
      "ACKNOWLEDGED",
      "INVESTIGATING",
      "RESOLVED",
    ];

    if (status !== undefined) {
      if (!allowedStatuses.includes(status)) {
        return res.status(400).json({
          success: false,
          message: "Invalid status",
        });
      }

      values.push(status);
      conditions.push(`status = $${values.length}`);
    }

    const whereClause =
      conditions.length > 0 ? `WHERE ${conditions.join(" AND ")}` : "";

    values.push(parsedLimit);

    const result = await pool.query(
      `SELECT *
       FROM incidents
       ${whereClause}
       ORDER BY created_at DESC, id DESC
       LIMIT $${values.length}`,
      values,
    );

    return res.json({
      success: true,
      count: result.rowCount,
      incidents: result.rows,
    });
  } catch (error) {
    console.error("Fetch incidents error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Get a single incident by ID
const getIncidentById = async (req, res) => {
  try {
    const { id } = req.params;

    if (!/^[1-9]\d*$/.test(id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid incident ID",
      });
    }

    const result = await pool.query("SELECT * FROM incidents WHERE id = $1", [
      id,
    ]);

    if (result.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "Incident not found",
      });
    }

    return res.json({
      success: true,
      incident: result.rows[0],
    });
  } catch (error) {
    console.error("Fetch incident error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

// Update incident status
const updateIncidentStatus = async (req, res) => {
  try {
    const { id } = req.params;
    const { status } = req.body;

    if (!/^[1-9]\d*$/.test(id)) {
      return res.status(400).json({
        success: false,
        message: "Invalid incident ID",
      });
    }

    const allowedStatuses = [
      "OPEN",
      "ACKNOWLEDGED",
      "INVESTIGATING",
      "RESOLVED",
    ];

    if (!allowedStatuses.includes(status)) {
      return res.status(400).json({
        success: false,
        message: "Invalid incident status",
      });
    }

    const currentResult = await pool.query(
      "SELECT * FROM incidents WHERE id = $1",
      [id],
    );

    if (currentResult.rowCount === 0) {
      return res.status(404).json({
        success: false,
        message: "Incident not found",
      });
    }

    const currentStatus = currentResult.rows[0].status;

    const allowedTransitions = {
      OPEN: ["ACKNOWLEDGED"],
      ACKNOWLEDGED: ["INVESTIGATING", "RESOLVED"],
      INVESTIGATING: ["RESOLVED"],
      RESOLVED: [],
    };

    if (
      currentStatus !== status &&
      !allowedTransitions[currentStatus].includes(status)
    ) {
      return res.status(400).json({
        success: false,
        message: `Cannot change incident status from ${currentStatus} to ${status}`,
      });
    }

    const result = await pool.query(
      `UPDATE incidents
   SET status = $1::VARCHAR,
       resolved_at = CASE
         WHEN $1::VARCHAR = 'RESOLVED'
         THEN COALESCE(resolved_at, NOW())
         ELSE NULL
       END
   WHERE id = $2
   RETURNING *`,
      [status, id],
    );

    return res.json({
      success: true,
      message: "Incident status updated successfully",
      incident: result.rows[0],
    });
  } catch (error) {
    console.error("Update incident status error:", error);

    return res.status(500).json({
      success: false,
      message: "Internal server error",
    });
  }
};

module.exports = {
  getIncidents,
  getIncidentById,
  updateIncidentStatus,
};
