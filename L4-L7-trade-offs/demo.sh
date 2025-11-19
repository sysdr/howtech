#!/bin/bash

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     L4 vs L7 Load Balancer Trade-offs Demo                     ║"
echo "║     Comparing performance for Video Streaming vs API Gateway   ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Create project structure
echo -e "${BLUE}[1/6] Creating project structure...${NC}"
mkdir -p backend/video-server backend/api-server dashboard haproxy metrics

# Create video chunk server (simulates video streaming backend)
cat > backend/video-server/server.js << 'EOF'
const http = require('http');
const os = require('os');

const PORT = process.env.PORT || 3001;
const SERVER_ID = process.env.SERVER_ID || 'video-1';

// Simulate video chunk (100KB of data)
const VIDEO_CHUNK = Buffer.alloc(100 * 1024, 'V');

let requestCount = 0;
let totalBytes = 0;

const server = http.createServer((req, res) => {
    const start = process.hrtime.bigint();
    requestCount++;
    
    // Handle CORS preflight requests
    if (req.method === 'OPTIONS') {
        res.writeHead(200, {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end();
        return;
    }
    
    if (req.url === '/health') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'healthy', server: SERVER_ID }));
        return;
    }
    
    if (req.url === '/metrics') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            server: SERVER_ID,
            requests: requestCount,
            bytesServed: totalBytes,
            uptime: process.uptime()
        }));
        return;
    }
    
    // Simulate video chunk delivery
    if (req.url.startsWith('/chunk')) {
        totalBytes += VIDEO_CHUNK.length;
        res.writeHead(200, {
            'Content-Type': 'video/mp4',
            'Content-Length': VIDEO_CHUNK.length,
            'X-Server-ID': SERVER_ID,
            'X-Processing-Time': '0',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end(VIDEO_CHUNK);
        
        const end = process.hrtime.bigint();
        const latencyUs = Number(end - start) / 1000;
        console.log(`[${SERVER_ID}] Chunk served in ${latencyUs.toFixed(0)}µs`);
        return;
    }
    
    res.writeHead(404);
    res.end('Not found');
});

server.listen(PORT, () => {
    console.log(`Video server ${SERVER_ID} running on port ${PORT}`);
});
EOF

# Create API server (simulates microservices)
cat > backend/api-server/server.js << 'EOF'
const http = require('http');

const PORT = process.env.PORT || 4001;
const SERVER_ID = process.env.SERVER_ID || 'api-1';
const SERVICE_TYPE = process.env.SERVICE_TYPE || 'users';

let requestCount = 0;

// Simulated data
const mockData = {
    users: [
        { id: 1, name: 'Alice', email: 'alice@example.com' },
        { id: 2, name: 'Bob', email: 'bob@example.com' }
    ],
    orders: [
        { id: 101, userId: 1, total: 99.99, status: 'shipped' },
        { id: 102, userId: 2, total: 149.99, status: 'pending' }
    ],
    products: [
        { id: 1001, name: 'Widget', price: 29.99, stock: 100 },
        { id: 1002, name: 'Gadget', price: 49.99, stock: 50 }
    ]
};

const server = http.createServer((req, res) => {
    const start = process.hrtime.bigint();
    requestCount++;
    
    res.setHeader('X-Server-ID', SERVER_ID);
    res.setHeader('X-Service-Type', SERVICE_TYPE);
    
    // Handle CORS preflight requests
    if (req.method === 'OPTIONS') {
        res.writeHead(200, {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end();
        return;
    }
    
    if (req.url === '/health') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'healthy', server: SERVER_ID, service: SERVICE_TYPE }));
        return;
    }
    
    if (req.url === '/metrics') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            server: SERVER_ID,
            service: SERVICE_TYPE,
            requests: requestCount,
            uptime: process.uptime()
        }));
        return;
    }
    
    // Route based on service type
    const data = mockData[SERVICE_TYPE] || [];
    
    // Simulate some processing time (1-5ms)
    const delay = Math.random() * 4 + 1;
    setTimeout(() => {
        res.writeHead(200, { 
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end(JSON.stringify({
            service: SERVICE_TYPE,
            server: SERVER_ID,
            data: data,
            timestamp: new Date().toISOString()
        }));
        
        const end = process.hrtime.bigint();
        const latencyMs = Number(end - start) / 1000000;
        console.log(`[${SERVER_ID}] ${SERVICE_TYPE} request processed in ${latencyMs.toFixed(2)}ms`);
    }, delay);
});

server.listen(PORT, () => {
    console.log(`API server ${SERVER_ID} (${SERVICE_TYPE}) running on port ${PORT}`);
});
EOF

# Create HAProxy L4 configuration (HTTP mode with CORS)
cat > haproxy/haproxy-l4.cfg << 'EOF'
global
    log stdout format raw local0
    maxconn 4096

defaults
    log global
    mode http
    option httplog
    timeout connect 5s
    timeout client 30s
    timeout server 30s

frontend video_frontend
    bind *:8080
    
    # CORS headers
    http-response set-header Access-Control-Allow-Origin "*"
    http-response set-header Access-Control-Allow-Methods "GET, POST, OPTIONS"
    http-response set-header Access-Control-Allow-Headers "Content-Type, X-Server-ID"
    
    # Handle OPTIONS preflight requests
    http-request return status 200 hdr Access-Control-Allow-Origin "*" hdr Access-Control-Allow-Methods "GET, POST, OPTIONS" hdr Access-Control-Allow-Headers "Content-Type, X-Server-ID" if METH_OPTIONS
    
    default_backend video_servers

backend video_servers
    balance roundrobin
    server video1 video-server-1:3001 check
    server video2 video-server-2:3001 check
    server video3 video-server-3:3001 check

listen stats
    bind *:8404
    mode http
    stats enable
    stats uri /stats
    stats refresh 5s
EOF

# Create HAProxy L7 configuration (HTTP mode)
cat > haproxy/haproxy-l7.cfg << 'EOF'
global
    log stdout format raw local0
    maxconn 4096

defaults
    log global
    mode http
    option httplog
    timeout connect 5s
    timeout client 30s
    timeout server 30s

frontend api_frontend
    bind *:8081
    
    # CORS headers
    http-response set-header Access-Control-Allow-Origin "*"
    http-response set-header Access-Control-Allow-Methods "GET, POST, OPTIONS"
    http-response set-header Access-Control-Allow-Headers "Content-Type, X-Server-ID"
    
    # Handle OPTIONS preflight requests
    http-request return status 200 hdr Access-Control-Allow-Origin "*" hdr Access-Control-Allow-Methods "GET, POST, OPTIONS" hdr Access-Control-Allow-Headers "Content-Type, X-Server-ID" if METH_OPTIONS
    
    # URL-based routing (L7 capability)
    acl is_users path_beg /users
    acl is_orders path_beg /orders
    acl is_products path_beg /products
    
    use_backend users_service if is_users
    use_backend orders_service if is_orders
    use_backend products_service if is_products
    
    default_backend users_service

backend users_service
    balance roundrobin
    option httpchk GET /health
    server users1 api-users-1:4001 check
    server users2 api-users-2:4001 check

backend orders_service
    balance roundrobin
    option httpchk GET /health
    server orders1 api-orders-1:4001 check
    server orders2 api-orders-2:4001 check

backend products_service
    balance roundrobin
    option httpchk GET /health
    server products1 api-products-1:4001 check
    server products2 api-products-2:4001 check

listen stats
    bind *:8405
    stats enable
    stats uri /stats
    stats refresh 5s
EOF

# Create dashboard
cat > dashboard/index.html << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>L4 vs L7 Load Balancer Demo</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: #e0e0e0;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            font-size: 2.5rem;
            background: linear-gradient(90deg, #00d4ff, #7b68ee);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        
        .header p {
            color: #888;
            margin-top: 10px;
        }
        
        .container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            max-width: 1400px;
            margin: 0 auto;
        }
        
        .panel {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 16px;
            padding: 24px;
            border: 1px solid rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
        }
        
        .panel-header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 20px;
        }
        
        .panel-title {
            font-size: 1.3rem;
            font-weight: 600;
        }
        
        .l4-color { color: #00d4ff; }
        .l7-color { color: #7b68ee; }
        
        .badge {
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 0.75rem;
            font-weight: 600;
        }
        
        .badge-l4 {
            background: rgba(0, 212, 255, 0.2);
            color: #00d4ff;
        }
        
        .badge-l7 {
            background: rgba(123, 104, 238, 0.2);
            color: #7b68ee;
        }
        
        .metrics {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
            margin-bottom: 20px;
        }
        
        .metric {
            background: rgba(0, 0, 0, 0.3);
            padding: 15px;
            border-radius: 10px;
        }
        
        .metric-label {
            font-size: 0.75rem;
            color: #888;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .metric-value {
            font-size: 1.8rem;
            font-weight: 700;
            margin-top: 5px;
        }
        
        .metric-unit {
            font-size: 0.9rem;
            color: #666;
        }
        
        .controls {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        button {
            flex: 1;
            padding: 12px 20px;
            border: none;
            border-radius: 8px;
            font-size: 0.9rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .btn-primary {
            background: linear-gradient(90deg, #00d4ff, #7b68ee);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(0, 212, 255, 0.3);
        }
        
        .btn-secondary {
            background: rgba(255, 255, 255, 0.1);
            color: #e0e0e0;
        }
        
        .btn-secondary:hover {
            background: rgba(255, 255, 255, 0.2);
        }
        
        .log-container {
            background: rgba(0, 0, 0, 0.4);
            border-radius: 8px;
            padding: 15px;
            height: 200px;
            overflow-y: auto;
            font-family: 'Monaco', 'Menlo', monospace;
            font-size: 0.8rem;
        }
        
        .log-entry {
            padding: 4px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }
        
        .log-time {
            color: #666;
        }
        
        .log-server {
            color: #00d4ff;
        }
        
        .log-latency {
            color: #4ade80;
        }
        
        .comparison {
            grid-column: span 2;
            text-align: center;
        }
        
        .comparison-chart {
            display: flex;
            justify-content: center;
            align-items: flex-end;
            gap: 40px;
            height: 200px;
            margin-top: 20px;
        }
        
        .bar-container {
            text-align: center;
        }
        
        .bar {
            width: 80px;
            border-radius: 8px 8px 0 0;
            transition: height 0.5s ease;
        }
        
        .bar-l4 {
            background: linear-gradient(180deg, #00d4ff, #0099cc);
        }
        
        .bar-l7 {
            background: linear-gradient(180deg, #7b68ee, #5a4fcf);
        }
        
        .bar-label {
            margin-top: 10px;
            font-size: 0.9rem;
            font-weight: 600;
        }
        
        .bar-value {
            margin-top: 5px;
            font-size: 0.8rem;
            color: #888;
        }
        
        .status-indicator {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 8px;
        }
        
        .status-online {
            background: #4ade80;
            box-shadow: 0 0 10px rgba(74, 222, 128, 0.5);
        }
        
        .status-offline {
            background: #f87171;
        }
        
        .full-width {
            grid-column: span 2;
        }
        
        .info-box {
            background: rgba(0, 212, 255, 0.1);
            border-left: 3px solid #00d4ff;
            padding: 15px;
            border-radius: 0 8px 8px 0;
            margin-top: 15px;
        }
        
        .info-box h4 {
            color: #00d4ff;
            margin-bottom: 8px;
        }
        
        .info-box p {
            font-size: 0.85rem;
            color: #aaa;
            line-height: 1.5;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>L4 vs L7 Load Balancer Demo</h1>
        <p>Real-time performance comparison for video streaming and API gateway scenarios</p>
    </div>
    
    <div class="container">
        <!-- L4 Panel - Video Streaming -->
        <div class="panel">
            <div class="panel-header">
                <span class="panel-title l4-color">
                    <span class="status-indicator status-online"></span>
                    Layer 4 - Video Streaming
                </span>
                <span class="badge badge-l4">TCP Mode</span>
            </div>
            
            <div class="metrics">
                <div class="metric">
                    <div class="metric-label">Avg Latency</div>
                    <div class="metric-value l4-color" id="l4-latency">--</div>
                    <div class="metric-unit">microseconds</div>
                </div>
                <div class="metric">
                    <div class="metric-label">Throughput</div>
                    <div class="metric-value" id="l4-throughput">--</div>
                    <div class="metric-unit">req/sec</div>
                </div>
                <div class="metric">
                    <div class="metric-label">Total Requests</div>
                    <div class="metric-value" id="l4-requests">0</div>
                </div>
                <div class="metric">
                    <div class="metric-label">Data Served</div>
                    <div class="metric-value" id="l4-data">0</div>
                    <div class="metric-unit">MB</div>
                </div>
            </div>
            
            <div class="controls">
                <button class="btn-primary" onclick="runL4Test()">Run Video Test</button>
                <button class="btn-secondary" onclick="clearL4Logs()">Clear Logs</button>
            </div>
            
            <div class="log-container" id="l4-logs">
                <div class="log-entry">Ready to stream video chunks...</div>
            </div>
            
            <div class="info-box">
                <h4>Why L4 for Video?</h4>
                <p>No HTTP parsing overhead. Packets pass through at wire speed. Perfect for long-lived, high-bandwidth connections where routing decisions happen once.</p>
            </div>
        </div>
        
        <!-- L7 Panel - API Gateway -->
        <div class="panel">
            <div class="panel-header">
                <span class="panel-title l7-color">
                    <span class="status-indicator status-online"></span>
                    Layer 7 - API Gateway
                </span>
                <span class="badge badge-l7">HTTP Mode</span>
            </div>
            
            <div class="metrics">
                <div class="metric">
                    <div class="metric-label">Avg Latency</div>
                    <div class="metric-value l7-color" id="l7-latency">--</div>
                    <div class="metric-unit">milliseconds</div>
                </div>
                <div class="metric">
                    <div class="metric-label">Throughput</div>
                    <div class="metric-value" id="l7-throughput">--</div>
                    <div class="metric-unit">req/sec</div>
                </div>
                <div class="metric">
                    <div class="metric-label">Total Requests</div>
                    <div class="metric-value" id="l7-requests">0</div>
                </div>
                <div class="metric">
                    <div class="metric-label">Routes Used</div>
                    <div class="metric-value" id="l7-routes">0/3</div>
                </div>
            </div>
            
            <div class="controls">
                <button class="btn-primary" onclick="runL7Test()">Run API Test</button>
                <button class="btn-secondary" onclick="clearL7Logs()">Clear Logs</button>
            </div>
            
            <div class="log-container" id="l7-logs">
                <div class="log-entry">Ready to route API requests...</div>
            </div>
            
            <div class="info-box">
                <h4>Why L7 for APIs?</h4>
                <p>URL-based routing to different services. Header inspection for auth. Health checks that verify app logic. The overhead pays for itself in functionality.</p>
            </div>
        </div>
        
        <!-- Comparison Chart -->
        <div class="panel comparison">
            <div class="panel-header">
                <span class="panel-title">Performance Comparison</span>
            </div>
            
            <div class="comparison-chart">
                <div class="bar-container">
                    <div class="bar bar-l4" id="l4-bar" style="height: 20px;"></div>
                    <div class="bar-label">L4 Latency</div>
                    <div class="bar-value" id="l4-bar-value">-- µs</div>
                </div>
                <div class="bar-container">
                    <div class="bar bar-l7" id="l7-bar" style="height: 20px;"></div>
                    <div class="bar-label">L7 Latency</div>
                    <div class="bar-value" id="l7-bar-value">-- ms</div>
                </div>
            </div>
            
            <p style="margin-top: 20px; color: #888;">
                Note: L4 shows microseconds (µs), L7 shows milliseconds (ms). 
                1ms = 1000µs
            </p>
        </div>
        
        <!-- How to Use -->
        <div class="panel full-width">
            <div class="panel-header">
                <span class="panel-title">Understanding the Demo</span>
            </div>
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px;">
                <div>
                    <h4 style="color: #00d4ff; margin-bottom: 10px;">L4 Video Streaming Test</h4>
                    <p style="font-size: 0.85rem; color: #aaa; line-height: 1.6;">
                        Simulates fetching 100KB video chunks through an L4 (TCP) load balancer. 
                        No HTTP parsing - just raw packet forwarding. Watch the microsecond-level latency.
                    </p>
                </div>
                <div>
                    <h4 style="color: #7b68ee; margin-bottom: 10px;">L7 API Gateway Test</h4>
                    <p style="font-size: 0.85rem; color: #aaa; line-height: 1.6;">
                        Routes requests to /users, /orders, /products based on URL path inspection.
                        The L7 load balancer parses HTTP and routes intelligently. Note the millisecond latency.
                    </p>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        let l4Requests = 0;
        let l7Requests = 0;
        let l4TotalLatency = 0;
        let l7TotalLatency = 0;
        let l4DataServed = 0;
        let routesUsed = new Set();
        
        const routes = ['/users', '/orders', '/products'];
        
        function addLog(container, message, type = '') {
            const log = document.getElementById(container);
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            const time = new Date().toLocaleTimeString();
            entry.innerHTML = `<span class="log-time">[${time}]</span> ${message}`;
            log.insertBefore(entry, log.firstChild);
            
            // Keep only last 50 entries
            while (log.children.length > 50) {
                log.removeChild(log.lastChild);
            }
        }
        
        async function runL4Test() {
            const iterations = 20;
            addLog('l4-logs', `Starting ${iterations} video chunk requests...`);
            
            for (let i = 0; i < iterations; i++) {
                const start = performance.now();
                
                try {
                    const response = await fetch('http://localhost:8080/chunk/' + i);
                    const data = await response.arrayBuffer();
                    
                    const latency = (performance.now() - start) * 1000; // Convert to microseconds
                    l4Requests++;
                    l4TotalLatency += latency;
                    l4DataServed += data.byteLength;
                    
                    const server = response.headers.get('X-Server-ID') || 'unknown';
                    addLog('l4-logs', 
                        `Chunk ${i+1}: <span class="log-server">${server}</span> - ` +
                        `<span class="log-latency">${latency.toFixed(0)}µs</span> - ${(data.byteLength/1024).toFixed(0)}KB`
                    );
                    
                    updateL4Metrics();
                } catch (err) {
                    addLog('l4-logs', `Error: ${err.message}`);
                }
                
                await new Promise(r => setTimeout(r, 50));
            }
            
            addLog('l4-logs', `Completed ${iterations} requests`);
        }
        
        async function runL7Test() {
            const iterations = 20;
            addLog('l7-logs', `Starting ${iterations} API requests...`);
            
            for (let i = 0; i < iterations; i++) {
                const route = routes[i % routes.length];
                const start = performance.now();
                
                try {
                    const response = await fetch('http://localhost:8081' + route);
                    const data = await response.json();
                    
                    const latency = performance.now() - start; // Already in milliseconds
                    l7Requests++;
                    l7TotalLatency += latency;
                    routesUsed.add(route);
                    
                    const server = response.headers.get('X-Server-ID') || 'unknown';
                    addLog('l7-logs', 
                        `${route}: <span class="log-server">${server}</span> - ` +
                        `<span class="log-latency">${latency.toFixed(2)}ms</span>`
                    );
                    
                    updateL7Metrics();
                } catch (err) {
                    addLog('l7-logs', `Error: ${err.message}`);
                }
                
                await new Promise(r => setTimeout(r, 50));
            }
            
            addLog('l7-logs', `Completed ${iterations} requests`);
        }
        
        function updateL4Metrics() {
            const avgLatency = l4Requests > 0 ? l4TotalLatency / l4Requests : 0;
            document.getElementById('l4-latency').textContent = avgLatency.toFixed(0);
            document.getElementById('l4-requests').textContent = l4Requests;
            document.getElementById('l4-data').textContent = (l4DataServed / (1024 * 1024)).toFixed(2);
            document.getElementById('l4-throughput').textContent = l4Requests > 0 ? 
                (l4Requests / (l4TotalLatency / 1000000)).toFixed(0) : '--';
            
            // Update chart
            const barHeight = Math.min(avgLatency / 50, 180);
            document.getElementById('l4-bar').style.height = barHeight + 'px';
            document.getElementById('l4-bar-value').textContent = avgLatency.toFixed(0) + ' µs';
        }
        
        function updateL7Metrics() {
            const avgLatency = l7Requests > 0 ? l7TotalLatency / l7Requests : 0;
            document.getElementById('l7-latency').textContent = avgLatency.toFixed(2);
            document.getElementById('l7-requests').textContent = l7Requests;
            document.getElementById('l7-routes').textContent = routesUsed.size + '/3';
            document.getElementById('l7-throughput').textContent = l7Requests > 0 ? 
                (l7Requests / (l7TotalLatency / 1000)).toFixed(0) : '--';
            
            // Update chart (convert ms to comparable scale)
            const barHeight = Math.min(avgLatency * 20, 180);
            document.getElementById('l7-bar').style.height = barHeight + 'px';
            document.getElementById('l7-bar-value').textContent = avgLatency.toFixed(2) + ' ms';
        }
        
        function clearL4Logs() {
            document.getElementById('l4-logs').innerHTML = '<div class="log-entry">Logs cleared</div>';
            l4Requests = 0;
            l4TotalLatency = 0;
            l4DataServed = 0;
            updateL4Metrics();
        }
        
        function clearL7Logs() {
            document.getElementById('l7-logs').innerHTML = '<div class="log-entry">Logs cleared</div>';
            l7Requests = 0;
            l7TotalLatency = 0;
            routesUsed.clear();
            updateL7Metrics();
        }
    </script>
</body>
</html>
EOF

# Create docker-compose.yml
cat > docker-compose.yml << 'EOF'
services:
  # Video streaming servers (L4 backends)
  video-server-1:
    build:
      context: ./backend/video-server
    environment:
      - SERVER_ID=video-1
      - PORT=3001
    networks:
      - lb-network

  video-server-2:
    build:
      context: ./backend/video-server
    environment:
      - SERVER_ID=video-2
      - PORT=3001
    networks:
      - lb-network

  video-server-3:
    build:
      context: ./backend/video-server
    environment:
      - SERVER_ID=video-3
      - PORT=3001
    networks:
      - lb-network

  # API microservices (L7 backends)
  api-users-1:
    build:
      context: ./backend/api-server
    environment:
      - SERVER_ID=users-1
      - SERVICE_TYPE=users
      - PORT=4001
    networks:
      - lb-network

  api-users-2:
    build:
      context: ./backend/api-server
    environment:
      - SERVER_ID=users-2
      - SERVICE_TYPE=users
      - PORT=4001
    networks:
      - lb-network

  api-orders-1:
    build:
      context: ./backend/api-server
    environment:
      - SERVER_ID=orders-1
      - SERVICE_TYPE=orders
      - PORT=4001
    networks:
      - lb-network

  api-orders-2:
    build:
      context: ./backend/api-server
    environment:
      - SERVER_ID=orders-2
      - SERVICE_TYPE=orders
      - PORT=4001
    networks:
      - lb-network

  api-products-1:
    build:
      context: ./backend/api-server
    environment:
      - SERVER_ID=products-1
      - SERVICE_TYPE=products
      - PORT=4001
    networks:
      - lb-network

  api-products-2:
    build:
      context: ./backend/api-server
    environment:
      - SERVER_ID=products-2
      - SERVICE_TYPE=products
      - PORT=4001
    networks:
      - lb-network

  # L4 Load Balancer (TCP mode)
  haproxy-l4:
    image: haproxy:2.8-alpine
    ports:
      - "8080:8080"
      - "8404:8404"
    volumes:
      - ./haproxy/haproxy-l4.cfg:/usr/local/etc/haproxy/haproxy.cfg:ro
    depends_on:
      - video-server-1
      - video-server-2
      - video-server-3
    networks:
      - lb-network

  # L7 Load Balancer (HTTP mode)
  haproxy-l7:
    image: haproxy:2.8-alpine
    ports:
      - "8081:8081"
      - "8405:8405"
    volumes:
      - ./haproxy/haproxy-l7.cfg:/usr/local/etc/haproxy/haproxy.cfg:ro
    depends_on:
      - api-users-1
      - api-users-2
      - api-orders-1
      - api-orders-2
      - api-products-1
      - api-products-2
    networks:
      - lb-network

  # Dashboard
  dashboard:
    image: nginx:alpine
    ports:
      - "3000:80"
    volumes:
      - ./dashboard:/usr/share/nginx/html:ro
    networks:
      - lb-network

networks:
  lb-network:
    driver: bridge
EOF

# Create Dockerfile for video server
cat > backend/video-server/Dockerfile << 'EOF'
FROM node:18-alpine
WORKDIR /app
COPY server.js .
EXPOSE 3001
CMD ["node", "server.js"]
EOF

# Create Dockerfile for API server
cat > backend/api-server/Dockerfile << 'EOF'
FROM node:18-alpine
WORKDIR /app
COPY server.js .
EXPOSE 4001
CMD ["node", "server.js"]
EOF

# Create cleanup script
cat > cleanup.sh << 'EOF'
#!/bin/bash

echo "Stopping and removing containers..."
docker-compose down

echo "Cleaning up..."
docker-compose rm -f

echo "Done!"
EOF

chmod +x cleanup.sh

# Create README.md
cat > README.md << 'EOF'
# L4 vs L7 Load Balancer Trade-offs Demo

This demo compares Layer 4 (TCP) and Layer 7 (HTTP) load balancers in different scenarios:

- **L4 Load Balancer**: Optimized for video streaming with minimal latency
- **L7 Load Balancer**: Optimized for API gateway with URL-based routing

## Quick Start

1. Run the setup script:
   ```bash
   ./demo.sh
   ```

2. Open the dashboard:
   http://localhost:3000

3. Test the load balancers:
   - L4 (Video): `curl http://localhost:8080/chunk/1`
   - L7 (API): `curl http://localhost:8081/users`

4. Clean up:
   ```bash
   ./cleanup.sh
   ```

## Architecture

- **Backend Servers**: Node.js servers simulating video streaming and API microservices
- **Load Balancers**: HAProxy configured for L4 (TCP) and L7 (HTTP) modes
- **Dashboard**: Real-time monitoring and testing interface

## Files Generated

- `backend/video-server/server.js` - Video streaming server
- `backend/api-server/server.js` - API microservice server
- `haproxy/haproxy-l4.cfg` - L4 load balancer configuration
- `haproxy/haproxy-l7.cfg` - L7 load balancer configuration
- `dashboard/index.html` - Web dashboard
- `docker-compose.yml` - Docker orchestration
- `cleanup.sh` - Cleanup script
EOF

echo -e "${BLUE}[2/6] Building Docker images...${NC}"
docker-compose build --quiet

echo -e "${BLUE}[3/6] Starting services...${NC}"
docker-compose up -d

echo -e "${BLUE}[4/6] Waiting for services to be healthy...${NC}"
sleep 5

# Check if services are running
echo -e "${BLUE}[5/6] Verifying services...${NC}"

check_service() {
    local name=$1
    local url=$2
    if curl -s "$url" > /dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} $name is running"
        return 0
    else
        echo -e "  ${RED}✗${NC} $name failed to start"
        return 1
    fi
}

check_service "L4 Load Balancer (HAProxy)" "http://localhost:8404/stats"
check_service "L7 Load Balancer (HAProxy)" "http://localhost:8405/stats"
check_service "Dashboard" "http://localhost:3000"

echo ""
echo -e "${BLUE}[6/6] Demo is ready!${NC}"
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Access Points:                                                ║"
echo "╠════════════════════════════════════════════════════════════════╣"
echo "║                                                                ║"
echo "║  📊 Dashboard:        http://localhost:3000                    ║"
echo "║                                                                ║"
echo "║  🔵 L4 Load Balancer: http://localhost:8080 (Video)            ║"
echo "║     Stats:            http://localhost:8404/stats              ║"
echo "║                                                                ║"
echo "║  🟣 L7 Load Balancer: http://localhost:8081 (API)              ║"
echo "║     Stats:            http://localhost:8405/stats              ║"
echo "║                                                                ║"
echo "╠════════════════════════════════════════════════════════════════╣"
echo "║  Test Commands:                                                ║"
echo "║                                                                ║"
echo "║  # Test L4 (video chunk):                                      ║"
echo "║  curl http://localhost:8080/chunk/1 -o /dev/null -w '%{time_total}s'     ║"
echo "║                                                                ║"
echo "║  # Test L7 (API routing):                                      ║"
echo "║  curl http://localhost:8081/users                              ║"
echo "║  curl http://localhost:8081/orders                             ║"
echo "║  curl http://localhost:8081/products                           ║"
echo "║                                                                ║"
echo "╠════════════════════════════════════════════════════════════════╣"
echo "║  To stop: ./cleanup.sh                                         ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}Open the dashboard and click the test buttons to see L4 vs L7 in action!${NC}"