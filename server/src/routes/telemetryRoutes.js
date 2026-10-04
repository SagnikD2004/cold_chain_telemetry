const express = require("express");

const {
  ingestTelemetry,
  getTelemetryHistory,
  getLatestTelemetry,
} = require("../controllers/telemetryController");

const router = express.Router();

router.post("/", ingestTelemetry);
router.get("/", getTelemetryHistory);
router.get("/latest/:node_id", getLatestTelemetry);

module.exports = router;
