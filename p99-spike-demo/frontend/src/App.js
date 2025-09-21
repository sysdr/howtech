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
        const url = isSlow ?  : ;

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
