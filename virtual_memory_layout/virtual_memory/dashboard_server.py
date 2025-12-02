#!/usr/bin/env python3
"""
Dashboard server for Virtual Memory Layout Explorer
Provides real-time metrics and demo functionality
"""

import os
import sys
import json
import time
import subprocess
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent.absolute()
BUILD_DIR = SCRIPT_DIR / "build"
OUTPUT_DIR = SCRIPT_DIR / "output"
LOG_DIR = SCRIPT_DIR / "logs"

class DashboardHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(self.get_dashboard_html().encode())
        elif self.path == '/api/metrics':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            metrics = self.get_metrics()
            self.wfile.write(json.dumps(metrics).encode())
        elif self.path == '/api/demo':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            demo_data = self.run_demo()
            self.wfile.write(json.dumps(demo_data).encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def log_message(self, format, *args):
        # Suppress default logging
        pass
    
    def get_metrics(self):
        """Collect current metrics"""
        metrics = {
            'timestamp': time.time(),
            'processes': {},
            'memory': {},
            'system': {}
        }
        
        # Get process info if demo is running
        demo_pid_file = LOG_DIR / "demo.pid"
        if demo_pid_file.exists():
            try:
                with open(demo_pid_file, 'r') as f:
                    pid = int(f.read().strip())
                
                # Get process stats
                stat_file = Path(f"/proc/{pid}/stat")
                if stat_file.exists():
                    with open(stat_file, 'r') as f:
                        stat_data = f.read().split()
                        if len(stat_data) >= 24:
                            metrics['processes']['demo'] = {
                                'pid': pid,
                                'vsize': int(stat_data[22]),  # Virtual memory size
                                'rss_pages': int(stat_data[23]),  # Resident set size
                                'state': stat_data[2]
                            }
                
                # Count memory regions
                maps_file = Path(f"/proc/{pid}/maps")
                if maps_file.exists():
                    with open(maps_file, 'r') as f:
                        regions = len(f.readlines())
                        metrics['processes']['demo']['memory_regions'] = regions
            except (ValueError, FileNotFoundError, IndexError):
                pass
        
        # Get system memory info
        try:
            with open('/proc/meminfo', 'r') as f:
                for line in f:
                    if line.startswith('MemTotal:'):
                        metrics['system']['mem_total'] = int(line.split()[1]) * 1024  # Convert KB to bytes
                    elif line.startswith('MemAvailable:'):
                        metrics['system']['mem_available'] = int(line.split()[1]) * 1024
                    elif line.startswith('MemFree:'):
                        metrics['system']['mem_free'] = int(line.split()[1]) * 1024
        except FileNotFoundError:
            pass
        
        return metrics
    
    def run_demo(self):
        """Run memexplore demo and return results"""
        demo_data = {
            'success': False,
            'output': '',
            'timestamp': time.time()
        }
        
        if not (BUILD_DIR / "memexplore").exists():
            demo_data['error'] = 'memexplore binary not found'
            return demo_data
        
        try:
            result = subprocess.run(
                [str(BUILD_DIR / "memexplore")],
                capture_output=True,
                text=True,
                timeout=5
            )
            demo_data['success'] = result.returncode == 0
            demo_data['output'] = result.stdout
            if result.stderr:
                demo_data['error'] = result.stderr
        except subprocess.TimeoutExpired:
            demo_data['error'] = 'Demo execution timeout'
        except Exception as e:
            demo_data['error'] = str(e)
        
        return demo_data
    
    def get_dashboard_html(self):
        """Generate dashboard HTML"""
        return """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Virtual Memory Layout Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            padding: 20px;
            min-height: 100vh;
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
        .dashboard {
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
            font-weight: 600;
            color: #666;
        }
        .metric-value {
            color: #333;
            font-family: 'Courier New', monospace;
        }
        .status {
            display: inline-block;
            padding: 5px 10px;
            border-radius: 5px;
            font-size: 0.9em;
            font-weight: bold;
        }
        .status.active {
            background: #4caf50;
            color: white;
        }
        .status.inactive {
            background: #f44336;
            color: white;
        }
        button {
            background: #667eea;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1em;
            margin: 10px 5px;
            transition: background 0.3s;
        }
        button:hover {
            background: #5568d3;
        }
        button:disabled {
            background: #ccc;
            cursor: not-allowed;
        }
        .demo-output {
            background: #f5f5f5;
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 15px;
            margin-top: 15px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            max-height: 400px;
            overflow-y: auto;
            white-space: pre-wrap;
        }
        .loading {
            text-align: center;
            color: #666;
            padding: 20px;
        }
        .error {
            color: #f44336;
            background: #ffebee;
            padding: 10px;
            border-radius: 5px;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔍 Virtual Memory Layout Dashboard</h1>
        
        <div class="dashboard">
            <div class="card">
                <h2>System Memory</h2>
                <div id="system-metrics" class="loading">Loading...</div>
            </div>
            
            <div class="card">
                <h2>Process Metrics</h2>
                <div id="process-metrics" class="loading">Loading...</div>
            </div>
            
            <div class="card">
                <h2>Demo Control</h2>
                <button onclick="runDemo()">Run Memory Explorer Demo</button>
                <div id="demo-output"></div>
            </div>
        </div>
    </div>
    
    <script>
        let updateInterval;
        
        function formatBytes(bytes) {
            if (bytes === 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
        }
        
        function updateMetrics() {
            fetch('/api/metrics')
                .then(response => response.json())
                .then(data => {
                    // Update system metrics
                    const systemDiv = document.getElementById('system-metrics');
                    if (data.system && Object.keys(data.system).length > 0) {
                        systemDiv.innerHTML = `
                            <div class="metric">
                                <span class="metric-label">Total Memory:</span>
                                <span class="metric-value">${formatBytes(data.system.mem_total || 0)}</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">Available:</span>
                                <span class="metric-value">${formatBytes(data.system.mem_available || 0)}</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">Free:</span>
                                <span class="metric-value">${formatBytes(data.system.mem_free || 0)}</span>
                            </div>
                        `;
                    } else {
                        systemDiv.innerHTML = '<div class="loading">No system data available</div>';
                    }
                    
                    // Update process metrics
                    const processDiv = document.getElementById('process-metrics');
                    if (data.processes && data.processes.demo) {
                        const demo = data.processes.demo;
                        processDiv.innerHTML = `
                            <div class="metric">
                                <span class="metric-label">Status:</span>
                                <span class="status active">Active</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">PID:</span>
                                <span class="metric-value">${demo.pid || 'N/A'}</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">Virtual Size:</span>
                                <span class="metric-value">${formatBytes(demo.vsize || 0)}</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">Resident Set:</span>
                                <span class="metric-value">${formatBytes((demo.rss_pages || 0) * 4096)}</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">Memory Regions:</span>
                                <span class="metric-value">${demo.memory_regions || 0}</span>
                            </div>
                            <div class="metric">
                                <span class="metric-label">State:</span>
                                <span class="metric-value">${demo.state || 'N/A'}</span>
                            </div>
                        `;
                    } else {
                        processDiv.innerHTML = `
                            <div class="metric">
                                <span class="metric-label">Status:</span>
                                <span class="status inactive">Inactive</span>
                            </div>
                            <div class="loading">No demo process running</div>
                        `;
                    }
                })
                .catch(error => {
                    console.error('Error fetching metrics:', error);
                });
        }
        
        function runDemo() {
            const outputDiv = document.getElementById('demo-output');
            outputDiv.innerHTML = '<div class="loading">Running demo...</div>';
            
            fetch('/api/demo')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        outputDiv.innerHTML = `<div class="demo-output">${escapeHtml(data.output)}</div>`;
                    } else {
                        outputDiv.innerHTML = `<div class="error">Error: ${escapeHtml(data.error || 'Unknown error')}</div>`;
                    }
                })
                .catch(error => {
                    outputDiv.innerHTML = `<div class="error">Error: ${escapeHtml(error.message)}</div>`;
                });
        }
        
        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
        
        // Update metrics every 2 seconds
        updateMetrics();
        updateInterval = setInterval(updateMetrics, 2000);
        
        // Cleanup on page unload
        window.addEventListener('beforeunload', () => {
            if (updateInterval) {
                clearInterval(updateInterval);
            }
        });
    </script>
</body>
</html>"""

def main():
    port = 8080
    server_address = ('', port)
    httpd = HTTPServer(server_address, DashboardHandler)
    
    print(f"Dashboard server starting on http://localhost:{port}")
    print("Press Ctrl+C to stop")
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down dashboard server...")
        httpd.shutdown()

if __name__ == '__main__':
    main()

