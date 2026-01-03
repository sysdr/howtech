#!/usr/bin/env python3
"""
Signal Handling Dashboard - Web-based monitoring dashboard
"""

import os
import sys
import time
import json
import subprocess
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime

class DashboardHandler(BaseHTTPRequestHandler):
    metrics = {
        'safe_signalfd_running': False,
        'dangerous_running': False,
        'monitor_running': False,
        'signals_received': 0,
        'last_signal_time': None,
        'processes': [],
        'uptime': 0
    }
    
    start_time = time.time()
    
    def log_message(self, format, *args):
        # Suppress default logging
        pass
    
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.serve_dashboard()
        elif self.path == '/metrics':
            self.serve_metrics()
        elif self.path == '/api/status':
            self.serve_status()
        else:
            self.send_error(404)
    
    def serve_dashboard(self):
        html = """<!DOCTYPE html>
<html>
<head>
    <title>Signal Handling Dashboard</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        h1 {
            color: white;
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .card {
            background: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            transition: transform 0.2s;
        }
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 6px 12px rgba(0,0,0,0.15);
        }
        .card h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.5em;
        }
        .metric {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #eee;
        }
        .metric:last-child {
            border-bottom: none;
        }
        .metric-label {
            font-weight: 500;
            color: #666;
        }
        .metric-value {
            font-weight: bold;
            color: #333;
        }
        .status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 0.9em;
            font-weight: bold;
        }
        .status.running {
            background: #4caf50;
            color: white;
        }
        .status.stopped {
            background: #f44336;
            color: white;
        }
        .demo-section {
            background: white;
            border-radius: 10px;
            padding: 20px;
            margin-top: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .demo-button {
            background: #667eea;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1em;
            margin: 5px;
            transition: background 0.3s;
        }
        .demo-button:hover {
            background: #5568d3;
        }
        .demo-button:disabled {
            background: #ccc;
            cursor: not-allowed;
        }
        .log {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 15px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            max-height: 300px;
            overflow-y: auto;
            margin-top: 15px;
        }
        .auto-refresh {
            text-align: center;
            color: white;
            margin-top: 20px;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚦 Signal Handling Dashboard</h1>
        
        <div class="grid">
            <div class="card">
                <h2>Process Status</h2>
                <div class="metric">
                    <span class="metric-label">Safe signalfd:</span>
                    <span class="metric-value" id="safe-status">-</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Dangerous:</span>
                    <span class="metric-value" id="danger-status">-</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Monitor:</span>
                    <span class="metric-value" id="monitor-status">-</span>
                </div>
            </div>
            
            <div class="card">
                <h2>Metrics</h2>
                <div class="metric">
                    <span class="metric-label">Signals Received:</span>
                    <span class="metric-value" id="signals-count">0</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Last Signal:</span>
                    <span class="metric-value" id="last-signal">Never</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Uptime:</span>
                    <span class="metric-value" id="uptime">0s</span>
                </div>
            </div>
            
            <div class="card">
                <h2>System Info</h2>
                <div class="metric">
                    <span class="metric-label">Active Processes:</span>
                    <span class="metric-value" id="process-count">0</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Build Status:</span>
                    <span class="metric-value" id="build-status">-</span>
                </div>
            </div>
        </div>
        
        <div class="demo-section">
            <h2>Demo Controls</h2>
            <button class="demo-button" onclick="startSafeDemo()">Start Safe Demo</button>
            <button class="demo-button" onclick="startDangerousDemo()">Start Dangerous Demo</button>
            <button class="demo-button" onclick="sendSignal()">Send Test Signal</button>
            <button class="demo-button" onclick="stopAll()">Stop All</button>
            <div class="log" id="log"></div>
        </div>
        
        <div class="auto-refresh">
            Auto-refreshing every 2 seconds | Last update: <span id="last-update">-</span>
        </div>
    </div>
    
    <script>
        let demoPid = null;
        
        function updateMetrics() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('safe-status').innerHTML = 
                        data.safe_signalfd_running ? 
                        '<span class="status running">Running</span>' : 
                        '<span class="status stopped">Stopped</span>';
                    document.getElementById('danger-status').innerHTML = 
                        data.dangerous_running ? 
                        '<span class="status running">Running</span>' : 
                        '<span class="status stopped">Stopped</span>';
                    document.getElementById('monitor-status').innerHTML = 
                        data.monitor_running ? 
                        '<span class="status running">Running</span>' : 
                        '<span class="status stopped">Stopped</span>';
                    document.getElementById('signals-count').textContent = data.signals_received;
                    document.getElementById('last-signal').textContent = 
                        data.last_signal_time || 'Never';
                    document.getElementById('uptime').textContent = Math.floor(data.uptime) + 's';
                    document.getElementById('process-count').textContent = data.processes.length;
                    document.getElementById('build-status').textContent = data.build_status || 'Unknown';
                    document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
                })
                .catch(e => console.error('Error:', e));
        }
        
        function addLog(msg) {
            const log = document.getElementById('log');
            const time = new Date().toLocaleTimeString();
            log.innerHTML += `[${time}] ${msg}<br>`;
            log.scrollTop = log.scrollHeight;
        }
        
        function startSafeDemo() {
            addLog('Starting safe signalfd demo...');
            // In a real implementation, this would start the process via API
        }
        
        function startDangerousDemo() {
            addLog('Starting dangerous demo...');
        }
        
        function sendSignal() {
            addLog('Sending test signal...');
        }
        
        function stopAll() {
            addLog('Stopping all demos...');
        }
        
        // Auto-refresh every 2 seconds
        setInterval(updateMetrics, 2000);
        updateMetrics();
    </script>
</body>
</html>"""
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(html.encode())
    
    def serve_metrics(self):
        self.update_metrics()
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(self.metrics).encode())
    
    def serve_status(self):
        self.update_metrics()
        status = {
            'safe_signalfd_running': self.metrics['safe_signalfd_running'],
            'dangerous_running': self.metrics['dangerous_running'],
            'monitor_running': self.metrics['monitor_running'],
            'signals_received': self.metrics['signals_received'],
            'last_signal_time': self.metrics['last_signal_time'],
            'uptime': time.time() - self.start_time,
            'processes': self.metrics['processes'],
            'build_status': self.check_build_status()
        }
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(status).encode())
    
    def update_metrics(self):
        # Check if processes are running
        script_dir = os.path.dirname(os.path.abspath(__file__))
        safe_path = os.path.join(script_dir, 'build', 'safe_signalfd')
        danger_path = os.path.join(script_dir, 'build', 'dangerous')
        monitor_path = os.path.join(script_dir, 'build', 'monitor')
        
        self.metrics['safe_signalfd_running'] = os.path.exists(safe_path) and os.access(safe_path, os.X_OK)
        self.metrics['dangerous_running'] = os.path.exists(danger_path) and os.access(danger_path, os.X_OK)
        self.metrics['monitor_running'] = os.path.exists(monitor_path) and os.access(monitor_path, os.X_OK)
        
        # Check for running processes
        try:
            result = subprocess.run(['pgrep', '-f', 'safe_signalfd'], 
                                  capture_output=True, text=True)
            self.metrics['safe_signalfd_running'] = result.returncode == 0
        except:
            pass
        
        try:
            result = subprocess.run(['pgrep', '-f', 'build/dangerous'], 
                                  capture_output=True, text=True)
            self.metrics['dangerous_running'] = result.returncode == 0
        except:
            pass
        
        # Count processes
        try:
            result = subprocess.run(['pgrep', '-f', 'build/'], 
                                  capture_output=True, text=True)
            pids = result.stdout.strip().split('\n') if result.stdout.strip() else []
            self.metrics['processes'] = [pid for pid in pids if pid]
        except:
            self.metrics['processes'] = []
    
    def check_build_status(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        required = ['build/safe_signalfd', 'build/dangerous', 'build/monitor']
        all_exist = all(os.path.exists(os.path.join(script_dir, f)) for f in required)
        return 'Built' if all_exist else 'Not Built'

def run_server(port=8080):
    server = HTTPServer(('', port), DashboardHandler)
    print(f"Dashboard server running on http://localhost:{port}")
    print("Press Ctrl+C to stop")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.shutdown()

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    run_server(port)

