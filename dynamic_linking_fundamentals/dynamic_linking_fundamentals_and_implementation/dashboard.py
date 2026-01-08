#!/usr/bin/env python3
"""
Dashboard server for Dynamic Linking Fundamentals Demo
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
from datetime import datetime

SCRIPT_DIR = Path(__file__).parent.absolute()
BUILD_DIR = SCRIPT_DIR / "build"
PLUGINS_DIR = SCRIPT_DIR / "plugins"
LOG_DIR = SCRIPT_DIR / "logs"

# Create logs directory
LOG_DIR.mkdir(exist_ok=True)

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
        """Get current metrics about the system"""
        metrics = {
            'timestamp': datetime.now().isoformat(),
            'executable_exists': (BUILD_DIR / "plugin_demo").exists(),
            'plugins': {},
            'last_demo_run': self.get_last_demo_time(),
            'system_info': {
                'platform': sys.platform,
                'python_version': sys.version.split()[0]
            }
        }
        
        # Check each plugin
        plugins = ['plugin_reverse.so', 'plugin_rot13.so', 'plugin_upper.so']
        for plugin in plugins:
            plugin_path = PLUGINS_DIR / plugin
            if plugin_path.exists():
                stat = plugin_path.stat()
                metrics['plugins'][plugin] = {
                    'exists': True,
                    'size': stat.st_size,
                    'modified': datetime.fromtimestamp(stat.st_mtime).isoformat()
                }
            else:
                metrics['plugins'][plugin] = {'exists': False}
        
        return metrics
    
    def get_last_demo_time(self):
        """Get timestamp of last demo run"""
        demo_log = LOG_DIR / "demo_output.log"
        if demo_log.exists():
            stat = demo_log.stat()
            return datetime.fromtimestamp(stat.st_mtime).isoformat()
        return None
    
    def run_demo(self):
        """Run the demo and return results"""
        demo_path = BUILD_DIR / "plugin_demo"
        if not demo_path.exists():
            return {'error': 'Demo executable not found'}
        
        try:
            # Run demo with RTLD_NOW
            result = subprocess.run(
                [str(demo_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            output = result.stdout + result.stderr
            
            # Save output to log
            with open(LOG_DIR / "demo_output.log", "w") as f:
                f.write(output)
            
            # Parse output for metrics
            metrics = self.parse_demo_output(output)
            metrics['success'] = result.returncode == 0
            metrics['output'] = output[-2000:]  # Last 2000 chars
            
            return metrics
        except subprocess.TimeoutExpired:
            return {'error': 'Demo execution timed out'}
        except Exception as e:
            return {'error': str(e)}
    
    def parse_demo_output(self, output):
        """Parse demo output to extract metrics"""
        metrics = {
            'plugins_loaded': [],
            'load_times': {},
            'test_results': {},
            'memory_mapped': False,
            'cleanup_successful': False
        }
        
        lines = output.split('\n')
        for i, line in enumerate(lines):
            # Extract plugin load times
            if 'Load time:' in line:
                parts = line.split('Load time:')
                if len(parts) > 1:
                    time_part = parts[1].strip()
                    # Extract cycles and microseconds
                    if 'CPU cycles' in time_part:
                        cycles = time_part.split('CPU cycles')[0].strip()
                        metrics['load_times'][f'plugin_{len(metrics["plugins_loaded"])}'] = {
                            'cycles': cycles,
                            'raw': time_part
                        }
            
            # Check for loaded plugins
            if "Loaded '" in line:
                plugin_name = line.split("Loaded '")[1].split("'")[0]
                metrics['plugins_loaded'].append(plugin_name)
            
            # Check for test outputs
            if 'Output:' in line:
                output_val = line.split('Output:')[1].strip().strip('"')
                plugin_idx = len(metrics['test_results'])
                metrics['test_results'][f'plugin_{plugin_idx}'] = output_val
            
            # Check for memory mappings
            if 'Memory Mappings' in line:
                metrics['memory_mapped'] = True
            
            # Check for cleanup
            if 'cleaned up' in line:
                metrics['cleanup_successful'] = True
        
        return metrics
    
    def get_dashboard_html(self):
        """Generate dashboard HTML"""
        return """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dynamic Linking Fundamentals Dashboard</title>
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
            max-width: 1400px;
            margin: 0 auto;
        }
        .header {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            margin-bottom: 20px;
            text-align: center;
        }
        .header h1 {
            color: #667eea;
            margin-bottom: 10px;
        }
        .metrics-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .card {
            background: white;
            padding: 25px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .card h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.2em;
            border-bottom: 2px solid #667eea;
            padding-bottom: 10px;
        }
        .metric-item {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #eee;
        }
        .metric-item:last-child {
            border-bottom: none;
        }
        .metric-label {
            font-weight: 600;
            color: #555;
        }
        .metric-value {
            color: #667eea;
            font-weight: 700;
        }
        .status-badge {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 0.9em;
            font-weight: 600;
        }
        .status-ok {
            background: #d4edda;
            color: #155724;
        }
        .status-error {
            background: #f8d7da;
            color: #721c24;
        }
        .btn {
            background: #667eea;
            color: white;
            border: none;
            padding: 15px 30px;
            border-radius: 5px;
            font-size: 1em;
            cursor: pointer;
            transition: background 0.3s;
            width: 100%;
            margin-top: 15px;
        }
        .btn:hover {
            background: #5568d3;
        }
        .btn:disabled {
            background: #ccc;
            cursor: not-allowed;
        }
        .output-box {
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 5px;
            padding: 15px;
            margin-top: 15px;
            max-height: 400px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            white-space: pre-wrap;
        }
        .loading {
            text-align: center;
            padding: 20px;
            color: #667eea;
        }
        .timestamp {
            color: #999;
            font-size: 0.9em;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔗 Dynamic Linking Fundamentals Dashboard</h1>
            <p>Real-time metrics and demo execution</p>
        </div>
        
        <div class="metrics-grid">
            <div class="card">
                <h2>System Status</h2>
                <div id="system-status">
                    <div class="loading">Loading...</div>
                </div>
            </div>
            
            <div class="card">
                <h2>Plugins</h2>
                <div id="plugins-status">
                    <div class="loading">Loading...</div>
                </div>
            </div>
            
            <div class="card">
                <h2>Demo Control</h2>
                <button class="btn" onclick="runDemo()">Run Demo (RTLD_NOW)</button>
                <button class="btn" onclick="runDemoLazy()" style="margin-top: 10px;">Run Demo (RTLD_LAZY)</button>
                <div id="demo-output" class="output-box" style="display: none;"></div>
            </div>
        </div>
        
        <div class="card" style="margin-top: 20px;">
            <h2>Live Metrics</h2>
            <div id="live-metrics">
                <div class="loading">Loading metrics...</div>
            </div>
            <div class="timestamp" id="last-update"></div>
        </div>
    </div>
    
    <script>
        let updateInterval;
        
        function updateMetrics() {
            fetch('/api/metrics')
                .then(r => r.json())
                .then(data => {
                    // Update system status
                    const systemStatus = document.getElementById('system-status');
                    systemStatus.innerHTML = `
                        <div class="metric-item">
                            <span class="metric-label">Executable</span>
                            <span class="status-badge ${data.executable_exists ? 'status-ok' : 'status-error'}">
                                ${data.executable_exists ? '✓ Found' : '✗ Missing'}
                            </span>
                        </div>
                        <div class="metric-item">
                            <span class="metric-label">Platform</span>
                            <span class="metric-value">${data.system_info.platform}</span>
                        </div>
                        <div class="metric-item">
                            <span class="metric-label">Python</span>
                            <span class="metric-value">${data.system_info.python_version}</span>
                        </div>
                        ${data.last_demo_run ? `
                        <div class="metric-item">
                            <span class="metric-label">Last Demo Run</span>
                            <span class="metric-value">${new Date(data.last_demo_run).toLocaleString()}</span>
                        </div>
                        ` : ''}
                    `;
                    
                    // Update plugins status
                    const pluginsStatus = document.getElementById('plugins-status');
                    let pluginsHtml = '';
                    for (const [name, info] of Object.entries(data.plugins)) {
                        pluginsHtml += `
                            <div class="metric-item">
                                <span class="metric-label">${name}</span>
                                <span class="status-badge ${info.exists ? 'status-ok' : 'status-error'}">
                                    ${info.exists ? '✓' : '✗'}
                                </span>
                            </div>
                        `;
                        if (info.exists) {
                            pluginsHtml += `
                                <div class="metric-item">
                                    <span class="metric-label">Size</span>
                                    <span class="metric-value">${(info.size / 1024).toFixed(2)} KB</span>
                                </div>
                            `;
                        }
                    }
                    pluginsStatus.innerHTML = pluginsHtml || '<div class="loading">No plugins found</div>';
                    
                    // Update timestamp
                    document.getElementById('last-update').textContent = 
                        `Last updated: ${new Date(data.timestamp).toLocaleString()}`;
                })
                .catch(err => {
                    console.error('Error fetching metrics:', err);
                });
        }
        
        function runDemo(lazy = false) {
            const outputBox = document.getElementById('demo-output');
            outputBox.style.display = 'block';
            outputBox.textContent = 'Running demo... Please wait...';
            
            fetch('/api/demo')
                .then(r => r.json())
                .then(data => {
                    if (data.error) {
                        outputBox.textContent = `Error: ${data.error}`;
                    } else {
                        let output = 'Demo Results:\\n\\n';
                        if (data.plugins_loaded) {
                            output += `Plugins Loaded: ${data.plugins_loaded.join(', ')}\\n\\n`;
                        }
                        if (data.test_results) {
                            output += 'Test Outputs:\\n';
                            for (const [key, value] of Object.entries(data.test_results)) {
                                output += `  ${key}: ${value}\\n`;
                            }
                            output += '\\n';
                        }
                        output += '\\nFull Output:\\n';
                        output += data.output || 'No output';
                        outputBox.textContent = output;
                    }
                })
                .catch(err => {
                    outputBox.textContent = `Error: ${err.message}`;
                });
        }
        
        function runDemoLazy() {
            runDemo(true);
        }
        
        // Initial load
        updateMetrics();
        
        // Update every 5 seconds
        updateInterval = setInterval(updateMetrics, 5000);
    </script>
</body>
</html>"""

def main():
    port = int(os.environ.get('PORT', 8080))
    server = HTTPServer(('0.0.0.0', port), DashboardHandler)
    print(f"Dashboard server starting on http://0.0.0.0:{port}")
    print(f"Open http://localhost:{port} in your browser")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down dashboard server...")
        server.shutdown()

if __name__ == '__main__':
    main()

