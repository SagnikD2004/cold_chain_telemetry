
const express = require("express");

const {
  getIncidents,
  getIncidentById,
  updateIncidentStatus,
} = require("../controllers/incidentController");

const router = express.Router();

router.get("/", getIncidents);
router.get("/:id", getIncidentById);
router.patch("/:id/status", updateIncidentStatus);

module.exports = router;

