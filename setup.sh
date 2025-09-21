#!/bin/bash
set -e

# --- 1. Project Setup and File Creation ---
echo "--- 🛠️  Setting up project structure and files... ---"
mkdir -p p99-spike-demo/{backend,frontend}
touch p99-spike-demo/backend/app.py
touch p99-spike-demo/backend/requirements.txt
touch p99-spike-demo/frontend/package.json
touch p99-spike-demo/frontend/src/index.js
touch p99-spike-demo/frontend/src/App.js
touch p99-spike-demo/frontend/src/index.css
touch p99-spike-demo/frontend/public/index.html

# --- 2. Python Backend Code (backend/app.py) ---
echo "--- 📄 Creating backend code... ---"
cat << EOF > p99-spike-demo/backend/app.py
from flask import Flask, jsonify
from flask_cors import CORS
import time
import random

app = Flask(__name__)
CORS(app)

@app.route('/fast')
def fast_endpoint():
    return jsonify({"message": "Fast response!"})

@app.route('/slow')
def slow_endpoint():
    time.sleep(0.5)  # Simulate a 500ms delay
    return jsonify({"message": "Slow response!"})

if __name__ == '__main__':
    app.run(port=5000, debug=True)
EOF

# --- 3. Python Backend Requirements ---
echo "--- 📦 Creating backend requirements file... ---"
cat << EOF > p99-spike-demo/backend/requirements.txt
Flask
Flask-Cors
EOF

# --- 4. React Frontend Code ---
echo "--- 📄 Creating frontend code... ---"
# package.json
cat << EOF > p99-spike-demo/frontend/package.json
{
  "name": "p99-spike-frontend",
  "version": "1.0.0",
  "private": true,
  "dependencies": {
    "react": "^18.3.1",
    "react-dom": "^18.3.1",
    "react-scripts": "5.0.1",
    "recharts": "^2.12.7"
  },
  "scripts": {
    "start": "react-scripts start",
    "build": "react-scripts build",
    "test": "react-scripts test"
  },
  "browserslist": {
    "production": [
      ">0.2%",
      "not dead",
      "not op_mini all"
    ],
    "development": [
      "last 1 chrome version",
      "last 1 firefox version",
      "last 1 safari version"
    ]
  }
}
EOF

# src/index.js
cat << EOF > p99-spike-demo/frontend/src/index.js
import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import App from './App';

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
EOF

# src/App.js
cat << EOF > p99-spike-demo/frontend/src/App.js
import React, { useState } from 'react';
import './index.css';

const API_BASE_URL = 'http://localhost:5000';
const NUM_REQUESTS = 1000;
const CONCURRENCY = 50;

function App() {
  const [p50, setP50] = useState(null);
  const [p90, setP90] = useState(null);
  const [p99, setP99] = useState(null);
  const [loading, setLoading] = useState(false);
  const [message, setMessage] = useState('Click "Run Test" to start');

  const runTest = async () => {
    setLoading(true);
    setMessage('Running test...');
    setP50(null);
    setP90(null);
    setP99(null);

    const latencies = [];
    const queue = [];
    let activeWorkers = 0;
    let requestsCompleted = 0;

    const worker = async () => {
      while (requestsCompleted < NUM_REQUESTS) {
        requestsCompleted++;
        const isSlow = Math.random() < 0.01; // 1% chance for a slow request
        const url = isSlow ? `${API_BASE_URL}/slow` : `${API_BASE_URL}/fast`;

        const start = performance.now();
        try {
          await fetch(url);
          const end = performance.now();
          latencies.push(end - start);
        } catch (error) {
          // Ignore failed requests for this demo
        }
      }
    };

    const workers = [];
    for (let i = 0; i < CONCURRENCY; i++) {
      workers.push(worker());
    }

    await Promise.all(workers);

    const sortedLatencies = latencies.sort((a, b) => a - b);
    
    const p50Val = sortedLatencies[Math.floor(sortedLatencies.length * 0.50)];
    const p90Val = sortedLatencies[Math.floor(sortedLatencies.length * 0.90)];
    const p99Val = sortedLatencies[Math.floor(sortedLatencies.length * 0.99)];
    
    setP50(p50Val ? p50Val.toFixed(2) : 'N/A');
    setP90(p90Val ? p90Val.toFixed(2) : 'N/A');
    setP99(p99Val ? p99Val.toFixed(2) : 'N/A');

    setLoading(false);
    setMessage('Test Complete!');
  };

  return (
    <div className="container">
      <h1 className="title">p99 Latency Spike Demo</h1>
      <p className="description">
        Observe how a small percentage of slow requests dramatically affects the 99th percentile latency.
      </p>
      <button onClick={runTest} disabled={loading} className="button">
        {loading ? 'Running...' : 'Run Test'}
      </button>
      <div className="metrics">
        <div className="metric-box green">
          <div className="metric-label">P50 Latency (ms)</div>
          <div className="metric-value">{p50 || '-'}</div>
        </div>
        <div className="metric-box orange">
          <div className="metric-label">P90 Latency (ms)</div>
          <div className="metric-value">{p90 || '-'}</div>
        </div>
        <div className="metric-box red">
          <div className="metric-label">P99 Latency (ms)</div>
          <div className="metric-value">{p99 || '-'}</div>
        </div>
      </div>
      <p className="status-message">{message}</p>
    </div>
  );
}

export default App;
EOF

# src/index.css
cat << EOF > p99-spike-demo/frontend/src/index.css
body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
  margin: 0;
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 100vh;
  background-color: #f0f2f5;
  color: #333;
}

.container {
  text-align: center;
  background: #fff;
  padding: 3rem;
  border-radius: 12px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
  max-width: 800px;
  width: 90%;
}

.title {
  font-size: 2.5rem;
  color: #2c3e50;
  margin-bottom: 0.5rem;
}

.description {
  font-size: 1.1rem;
  color: #7f8c8d;
  margin-bottom: 2rem;
}

.button {
  background-color: #3498db;
  color: white;
  border: none;
  padding: 15px 30px;
  font-size: 1rem;
  font-weight: bold;
  border-radius: 8px;
  cursor: pointer;
  transition: background-color 0.3s, transform 0.1s;
}

.button:hover:not(:disabled) {
  background-color: #2980b9;
}

.button:disabled {
  background-color: #95a5a6;
  cursor: not-allowed;
}

.metrics {
  display: flex;
  justify-content: space-around;
  margin-top: 2rem;
  flex-wrap: wrap;
}

.metric-box {
  background-color: #ecf0f1;
  padding: 1.5rem;
  border-radius: 8px;
  margin: 10px;
  min-width: 180px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
}

.metric-box.green {
  border-left: 5px solid #27ae60;
}

.metric-box.orange {
  border-left: 5px solid #f39c12;
}

.metric-box.red {
  border-left: 5px solid #e74c3c;
}

.metric-label {
  font-size: 0.9rem;
  color: #7f8c8d;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.metric-value {
  font-size: 2.5rem;
  font-weight: 700;
  color: #2c3e50;
  margin-top: 0.5rem;
}

.status-message {
  margin-top: 2rem;
  font-style: italic;
  color: #95a5a6;
}
EOF

# public/index.html
cat << EOF > p99-spike-demo/frontend/public/index.html
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>P99 Latency Demo</title>
  </head>
  <body>
    <noscript>You need to enable JavaScript to run this app.</noscript>
    <div id="root"></div>
  </body>
</html>
EOF

# --- 5. Build and Run Demonstration ---
echo "--- 🚀 Building and running the demo... ---"
# Run backend with Docker Compose
echo "--- Starting backend service with Docker... ---"
cat << EOF > p99-spike-demo/docker-compose.yml
version: '3.8'
services:
  backend:
    build:
      context: ./backend
    ports:
      - "5000:5000"
    command: python app.py
EOF

cat << EOF > p99-spike-demo/backend/Dockerfile
FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .
CMD ["python", "app.py"]
EOF

docker-compose -f p99-spike-demo/docker-compose.yml up -d --build
sleep 5 # Give the backend a moment to start

echo "--- 📦 Installing frontend dependencies... ---"
cd p99-spike-demo/frontend
npm install

echo "--- 🚀 Starting React frontend... ---"
# Start the frontend in the background and a separate terminal command for user
npm start &
FRONTEND_PID=$!
sleep 5
echo "--- ✅ Demo is running! Visit http://localhost:3000 to see the application. ---"
echo "After you are done, run 'docker-compose -f p99-spike-demo/docker-compose.yml down' to stop the backend."

echo "--- To run without Docker, use these commands: ---"
echo "--- Backend: (from p99-spike-demo/backend) python -m venv venv && source venv/bin/activate && pip install -r requirements.txt && python app.py"
echo "--- Frontend: (from p99-spike-demo/frontend) npm install && npm start"

echo "--- Verification and Test ---"
echo "Functional test: "
echo "1. The web page at http://localhost:3000 should load and display 'p99 Latency Spike Demo'."
echo "2. Click the 'Run Test' button. It should change to 'Running...'. "
echo "3. After a few seconds, the P50, P90, and P99 metrics should update. You should see a large difference, with P99 being significantly higher (around 500ms)."
echo "4. The status message should change to 'Test Complete!'. This verifies the client-server interaction and the metric calculation."

echo "--- Finalizing... ---"
echo "To clean up, you need to manually stop the frontend process (kill $FRONTEND_PID) and run 'docker-compose -f p99-spike-demo/docker-compose.yml down'."