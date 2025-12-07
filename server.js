const express = require("express");
const cors = require("cors");
const { exec } = require("child_process");
const path = require("path");

const app = express();
app.use(cors());
app.use(express.json());

// ---------------------------
// Path to C backend executable
// ---------------------------
// On Windows: "triage.exe"
// On Linux/Mac: "triage" (make sure it is compiled)
const exePath = path.join(__dirname, "triage.exe");

// ---------------------------
// Utility: Convert C output → JSON
// ---------------------------
function parsePatients(data) {
  const lines = data.trim().split("\n");
  const patients = [];

  lines.forEach(line => {
    // Expected C output format: id name age severity
    const parts = line.trim().split(" ");
    if (parts.length >= 4) {
      patients.push({
        id: Number(parts[0]),
        name: parts[1],
        age: Number(parts[2]),
        severity: Number(parts[3])
      });
    }
  });

  return patients;
}

// ---------------------------
// API ENDPOINTS
// ---------------------------

// Get all patients
app.get("/patients", (req, res) => {
  exec(`"${exePath}" list`, (err, stdout) => {
    if (err) return res.status(500).json({ error: "C backend error" });
    return res.json(parsePatients(stdout));
  });
});

// Add patient
app.post("/addPatient", (req, res) => {
  const { id, name, age, severity } = req.body;
  exec(`"${exePath}" add ${id} "${name}" ${age} ${severity}`, (err) => {
    if (err) return res.status(500).json({ error: "Add failed" });
    return res.json({ ok: true });
  });
});

// Update patient severity
app.put("/updatePatient", (req, res) => {
  const { id, severity } = req.body;
  exec(`"${exePath}" update ${id} ${severity}`, (err) => {
    if (err) return res.status(500).json({ error: "Update failed" });
    return res.json({ ok: true });
  });
});

// Delete patient
app.delete("/deletePatient/:id", (req, res) => {
  const id = req.params.id;
  exec(`"${exePath}" delete ${id}`, (err) => {
    if (err) return res.status(500).json({ error: "Delete failed" });
    return res.json({ ok: true });
  });
});

// ---------------------------
// Serve frontend
// ---------------------------
app.use(express.static(path.join(__dirname, "hospital-triage", "public")));

// Catch-all route for SPA
app.get("*", (req, res) => {
  res.sendFile(path.join(__dirname, "hospital-triage", "public", "index.html"));
});

// ---------------------------
// START SERVER
// ---------------------------
const port = process.env.PORT || 5000;
app.listen(port, () => {
  console.log(`Backend running at http://localhost:${port}`);
});

