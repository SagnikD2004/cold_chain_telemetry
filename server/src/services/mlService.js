async function evaluateML(reading) {
  // Future integration point for the ML anomaly detection model.
  // No inference is performed at this stage.

  return {
    evaluated: false,
    mse: null,
    anomaly: null,
    status: "NOT_IMPLEMENTED",
  };
}

module.exports = {
  evaluateML,
};
