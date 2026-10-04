const API_URL = "http://localhost:8000/api/telemetry";

const NODE_ID = "ESP32-CC-001";
const TRIP_ID = 1;

let seqCounter = 1;
let temperature = 5.0;

function randomBetween(min, max) {
  return Math.random() * (max - min) + min;
}

function generateReading() {
  const event = Math.random();

  let temperatureChange;
  let vibration;

  if (event < 0.15) {
    // Simulate temperature excursion
    temperatureChange = randomBetween(3, 6);
    vibration = randomBetween(0.2, 1.5);
  } else if (event < 0.25) {
    // Simulate vibration spike
    temperatureChange = randomBetween(-0.5, 0.5);
    vibration = randomBetween(2.6, 4.0);
  } else {
    // Normal operating conditions
    temperatureChange = randomBetween(-0.8, 0.8);
    vibration = randomBetween(0.1, 1.8);
  }

  temperature += temperatureChange;

  // Gradually return temperature toward normal range
  if (temperature > 10) {
    temperature -= randomBetween(1, 2);
  }

  if (temperature < 2) {
    temperature += randomBetween(0.5, 1.2);
  }

  return {
    node_id: NODE_ID,
    trip_id: TRIP_ID,
    temperature_c: Number(temperature.toFixed(2)),
    vibration_rms: Number(vibration.toFixed(2)),
    recorded_at: new Date().toISOString(),
    seq_counter: seqCounter++,
  };
}

async function sendTelemetry() {
  const reading = generateReading();

  try {
    const response = await fetch(API_URL, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(reading),
    });

    const result = await response.json();

    if (!response.ok) {
      console.error("Request failed:", response.status, result);
      return;
    }

    console.log(
      `[${new Date().toLocaleTimeString()}]`,
      `Temp: ${reading.temperature_c}°C`,
      `Vibration: ${reading.vibration_rms}g`,
      `Seq: ${reading.seq_counter}`,
      `Incidents: ${result.incidents?.length ?? 0}`
    );
  } catch (error) {
    console.error("Connection error:", error.message);
  }
}

console.log("Cold-chain telemetry simulator started");
console.log(`Device: ${NODE_ID}`);
console.log(`Trip ID: ${TRIP_ID}`);
console.log("Sending readings every 3 seconds...");
console.log("Press Ctrl+C to stop.\n");

sendTelemetry();
setInterval(sendTelemetry, 3000);
