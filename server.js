const express = require("express");
const cors = require("cors");
const { exec } = require("child_process");
const path = require("path");
const fs = require("fs");

const app = express();
app.use(cors());
app.use(express.json());

// Detect correct C executable
let exePath = path.join(__dirname, "triage"); // Linux
if (fs.existsSync(path.join(__dirname, "triage.exe"))) {
  exePath = path.join(__dirname, "triage.exe"); // Windows
}

// GET all patients
app.get("/patients", (req, res) => {
  exec(`"${exePath}" list`, (err, stdout) => {
    if (err) {
      console.error("LIST ERROR:", err);
      return res.status(500).json({ error: "C backend error" });
    }

    try {
      const match = stdout.match(/\[.*\]/s); // extract JSON
      if (!match) return res.status(500).json({ error: "JSON not found", raw: stdout });

      const json = JSON.parse(match[0]).map(p => ({
        id: p.id,
        name: p.name || "Unknown",
        age: p.age,
        severity: p.severity
      }));

      return res.json(json);
    } catch (e) {
      console.error("JSON Parse Error:", stdout);
      return res.status(500).json({ error: "JSON parse failed", raw: stdout });
    }
  });
});

// ---------------------------
// ADD patient (fixed for multi-word names)
// ---------------------------
app.post("/addPatient", (req, res) => {
  const { id, name, age, severity } = req.body;
  if (!id || !name || !age || !severity) return res.status(400).json({ error: "Missing fields" });

  // Use exec instead of spawn to correctly handle multi-word names
  const cmd = `"${exePath}" add ${id} "${name}" ${age} ${severity}`;
  exec(cmd, (err, stdout, stderr) => {
    if (err) {
      console.error("ADD ERROR:", stderr);
      return res.status(500).json({ error: "Add failed", details: stderr });
    }
    return res.json({ ok: true });
  });
});

// UPDATE severity
app.put("/updatePatient", (req, res) => {
  const { id, severity } = req.body;
  if (!id || !severity) return res.status(400).json({ error: "Missing ID or Severity" });

  const cmd = `"${exePath}" update ${id} ${severity}`;
  exec(cmd, (err, stdout, stderr) => {
    if (err) {
      console.error("UPDATE ERROR:", stderr);
      return res.status(500).json({ error: "Update failed", details: stderr });
    }
    return res.json({ ok: true });
  });
});

// DELETE patient
app.delete("/deletePatient/:id", (req, res) => {
  const id = req.params.id;
  if (!id) return res.status(400).json({ error: "Missing ID" });

  const cmd = `"${exePath}" delete ${id}`;
  exec(cmd, (err, stdout, stderr) => {
    if (err) {
      console.error("DELETE ERROR:", stderr);
      return res.status(500).json({ error: "Delete failed", details: stderr });
    }
    return res.json({ ok: true });
  });
});

// Serve Frontend
const publicPath = path.join(__dirname, "public");
app.use(express.static(publicPath));

// SPA Fallback
app.get("*", (req, res) => {
  res.sendFile(path.join(publicPath, "index.html"));
});

// Start Server
const port = process.env.PORT || 5000;
app.listen(port, () => {
  console.log(`Server running on port ${port}`);
});
