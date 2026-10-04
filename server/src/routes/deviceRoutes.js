const express = require("express");

const {
  registerDevice,
  getDevices,
  getDeviceById,
} = require("../controllers/deviceController");

const router = express.Router();

router.post("/", registerDevice);
router.get("/", getDevices);
router.get("/:id", getDeviceById);

module.exports = router;

