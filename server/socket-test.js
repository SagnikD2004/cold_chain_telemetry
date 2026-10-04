const { io } = require("socket.io-client");

const socket = io("http://localhost:8000");

socket.on("connect", () => {
  console.log("Connected to Socket.IO server");
  console.log("Socket ID:", socket.id);
});

socket.on("telemetry:new", (data) => {
  console.log("\nNew telemetry received:");
  console.log(data.telemetry);
});

socket.on("incident:new", (data) => {
  console.log("\nNew incident received:");
  console.log(data.incident);
});

socket.on("connect_error", (error) => {
  console.error("Connection error:", error.message);
});

socket.on("disconnect", (reason) => {
  console.log("Disconnected:", reason);
});
