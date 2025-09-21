// index.js - simple express app exposing /health and sample endpoints
const express = require('express');
const app = express();
const port = process.env.PORT || 3000;

// simple in-memory store for simulating p99 spike
let simulateSlow = false;

app.get('/health', (req, res) => res.json({status: 'ok', uptime: process.uptime()}));

app.get('/toggle-slow', (req, res) => {
  simulateSlow = !simulateSlow;
  res.json({simulateSlow});
});

app.get('/api/data', (req, res) => {
  const start = Date.now();
  if (simulateSlow && Math.random() < 0.1) {
    // randomly slow 10% of requests to emulate p99 spike
    const delay = 300 + Math.floor(Math.random() * 400);
    setTimeout(() => res.json({data: 'ok', delay, t: Date.now() - start}), delay);
  } else {
    // fast path
    res.json({data: 'ok', t: Date.now() - start});
  }
});

app.listen(port, () => console.log(`Service listening on ${port}`));
