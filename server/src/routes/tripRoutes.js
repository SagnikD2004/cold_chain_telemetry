const express = require("express");

const {
  createTrip,
  getTrips,
  getTripById,
  updateTripStatus,
  getTripCompliance,
} = require("../controllers/tripController");

const router = express.Router();

router.post("/", createTrip);
router.get("/", getTrips);
router.get("/:id", getTripById);
router.patch("/:id/status", updateTripStatus);
router.get("/:id/compliance", getTripCompliance);

module.exports = router;
