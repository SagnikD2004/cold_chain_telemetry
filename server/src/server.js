require("dotenv").config();

const http = require("http");
const app = require("./app");
const pool = require("./config/db");
const { initializeSocket } = require("./config/socket");

const PORT = process.env.PORT || 8000;

// Create HTTP server
const server = http.createServer(app);

// Initialize Socket.IO
initializeSocket(server);

async function startServer() {
  try {
    await pool.query("SELECT NOW()");

    console.log("PostgreSQL connected successfully");

    server.listen(PORT, () => {
      console.log(`Server running on http://localhost:${PORT}`);
      console.log("Socket.IO server initialized");
    });
  } catch (error) {
    console.error("Server startup failed:", error.message);
    process.exit(1);
  }
}

startServer();
