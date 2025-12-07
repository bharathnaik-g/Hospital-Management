const express = require("express");
const cors = require("cors");
const { exec } = require("child_process");
const path = require("path");
const fs = require("fs");

const app = express();
app.use(cors());
app.use(express.json());

// --------------------------------------------------
// Detect correct C executable (Linux on Render / Windows locally)
// --------------------------------------------------
let exePath = path.join(__dirname, "triage"); // Linux
if (fs.existsSync(path.join(__dirname, "triage.exe"))) {
  exePath = path.join(__dirname, "triage.exe"); // Windows
}

// --------------------------------------------------
// API ENDPOINTS
// --------------------------------------------------

// GET all patients
app.get("/patients", (req, res) => {
  exec(`"${exePath}" list`, (err, stdout) => {
    if (err) {
      console.error("LIST ERROR:", err);
      return res.status(500).json({ error: "C backend error" });
    }

    try {
      const json = JSON.parse(stdout);
      return res.json(json);
    } catch (e) {
      console.error("JSON Parse Error:", stdout);
      return res.status(500).json({
        error: "JSON parse failed",
        raw: stdout
      });
    }
  });
});

// ADD patient
app.post("/addPatient", (req, res) => {
  const { id, name, age, severity } = req.body;

  exec(`"${exePath}" add ${id} "${name}" ${age} ${severity}`, (err) => {
    if (err) {
      console.error("ADD ERROR:", err);
      return res.status(500).json({ error: "Add failed" });
    }
    return res.json({ ok: true });
  });
});

// UPDATE severity
app.put("/updatePatient", (req, res) => {
  const { id, severity } = req.body;

  exec(`"${exePath}" update ${id} ${severity}`, (err) => {
    if (err) {
      console.error("UPDATE ERROR:", err);
      return res.status(500).json({ error: "Update failed" });
    }
    return res.json({ ok: true });
  });
});

// DELETE patient
app.delete("/deletePatient/:id", (req, res) => {
  const id = req.params.id;

  exec(`"${exePath}" delete ${id}`, (err) => {
    if (err) {
      console.error("DELETE ERROR:", err);
      return res.status(500).json({ error: "Delete failed" });
    }
    return res.json({ ok: true });
  });
});

// --------------------------------------------------
// Serve Frontend (public folder)
// --------------------------------------------------
const publicPath = path.join(__dirname, "public");
app.use(express.static(publicPath));

// SPA Fallback (works on Render)
app.get("*", (req, res) => {
  res.sendFile(path.join(publicPath, "index.html"));
});

// --------------------------------------------------
// Start Server
// --------------------------------------------------
const port = process.env.PORT || 5000;
app.listen(port, () => {
  console.log(`Server running on port ${port}`);
});

