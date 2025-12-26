#!/usr/bin/env python3
"""
Kernel Module Debugging Dashboard
Real-time metrics and monitoring for kernel module debugging demo
"""

import os
import sys
import time
import json
import random
import subprocess
import threading
from datetime import datetime
from flask import Flask, render_template_string, jsonify, request
from collections import deque

app = Flask(__name__)

# Metrics storage
metrics = {
    'module_loaded': False,
    'log_messages': deque(maxlen=100),
    'log_levels': {'EMERG': 0, 'ALERT': 0, 'CRIT': 0, 'ERR': 0, 'WARNING': 0, 'NOTICE': 0, 'INFO': 0, 'DEBUG': 0},
    'message_count': 0,
    'last_update': datetime.now().isoformat(),
    'system_info': {}
}

# Demo mode flag
demo_mode = False

def check_module_loaded():
    """Check if debug_demo module is loaded"""
    try:
        result = subprocess.run(['lsmod'], capture_output=True, text=True, timeout=2)
        return 'debug_demo' in result.stdout
    except:
        return False

def get_kernel_logs():
    """Get recent kernel logs from debug_demo module"""
    try:
        result = subprocess.run(['dmesg'], capture_output=True, text=True, timeout=2)
        lines = result.stdout.split('\n')
        debug_lines = [line for line in lines if 'debug_demo:' in line]
        return debug_lines[-20:] if debug_lines else []
    except:
        return []

def parse_log_level(line):
    """Parse log level from kernel log line"""
    line_upper = line.upper()
    if 'EMERG' in line_upper or '<0>' in line:
        return 'EMERG'
    elif 'ALERT' in line_upper or '<1>' in line:
        return 'ALERT'
    elif 'CRIT' in line_upper or '<2>' in line:
        return 'CRIT'
    elif 'ERR' in line_upper or '<3>' in line:
        return 'ERR'
    elif 'WARNING' in line_upper or '<4>' in line:
        return 'WARNING'
    elif 'NOTICE' in line_upper or '<5>' in line:
        return 'NOTICE'
    elif 'INFO' in line_upper or '<6>' in line:
        return 'INFO'
    elif 'DEBUG' in line_upper or '<7>' in line:
        return 'DEBUG'
    return 'INFO'

def get_system_info():
    """Get system information"""
    info = {}
    try:
        # Kernel version
        with open('/proc/version', 'r') as f:
            info['kernel_version'] = f.read().strip()
        
        # Uptime
        with open('/proc/uptime', 'r') as f:
            uptime_seconds = float(f.read().split()[0])
            hours = int(uptime_seconds // 3600)
            minutes = int((uptime_seconds % 3600) // 60)
            info['uptime'] = f"{hours}h {minutes}m"
        
        # Load average
        with open('/proc/loadavg', 'r') as f:
            load = f.read().split()[:3]
            info['load_avg'] = ', '.join(load)
    except:
        pass
    return info

def generate_demo_logs():
    """Generate demo kernel log messages"""
    import random
    demo_logs = [
        "[  123.456789] debug_demo: INFO: Module initialized successfully",
        "[  124.123456] debug_demo: DEBUG: Setting up device structures",
        "[  125.234567] debug_demo: NOTICE: Device registered with major number 247",
        "[  126.345678] debug_demo: INFO: Opening device file /dev/debug_demo",
        "[  127.456789] debug_demo: DEBUG: Read operation requested, buffer size: 1024",
        "[  128.567890] debug_demo: WARNING: Buffer size exceeds recommended limit",
        "[  129.678901] debug_demo: INFO: Write operation completed, bytes: 512",
        "[  130.789012] debug_demo: DEBUG: Processing ioctl command 0x1234",
        "[  131.890123] debug_demo: ERR: Invalid parameter detected in ioctl",
        "[  132.901234] debug_demo: INFO: Recovering from error state",
        "[  133.012345] debug_demo: NOTICE: Device operations resumed normally",
        "[  134.123456] debug_demo: DEBUG: Closing device file descriptor",
        "[  135.234567] debug_demo: INFO: Module cleanup initiated",
        "[  136.345678] debug_demo: DEBUG: Releasing allocated resources",
        "[  137.456789] debug_demo: NOTICE: Module unloaded successfully",
    ]
    return random.sample(demo_logs, min(5, len(demo_logs)))

def update_metrics():
    """Update metrics in background thread"""
    global metrics, demo_mode
    while True:
        try:
            if demo_mode:
                # In demo mode, simulate module loaded and generate demo logs
                metrics['module_loaded'] = True
                
                # Occasionally add new demo logs
                if random.random() < 0.3:  # 30% chance each cycle
                    new_logs = generate_demo_logs()
                    for log in new_logs:
                        if log not in metrics['log_messages']:
                            metrics['log_messages'].append(log)
                            level = parse_log_level(log)
                            metrics['log_levels'][level] = metrics['log_levels'].get(level, 0) + 1
                            metrics['message_count'] += 1
            else:
                # Normal mode: check actual module status
                metrics['module_loaded'] = check_module_loaded()
                
                # Get kernel logs
                logs = get_kernel_logs()
                for log in logs:
                    if log not in metrics['log_messages']:
                        metrics['log_messages'].append(log)
                        level = parse_log_level(log)
                        metrics['log_levels'][level] = metrics['log_levels'].get(level, 0) + 1
                        metrics['message_count'] += 1
            
            # Update system info
            metrics['system_info'] = get_system_info()
            metrics['last_update'] = datetime.now().isoformat()
            
        except Exception as e:
            print(f"Error updating metrics: {e}", file=sys.stderr)
        
        time.sleep(2)  # Update every 2 seconds

# Start background thread
update_thread = threading.Thread(target=update_metrics, daemon=True)
update_thread.start()

# HTML Template
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kernel Module Debugging Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #2196F3 0%, #4CAF50 100%);
            color: #333;
            padding: 20px;
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
        }
        .header {
            background: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 {
            color: #2196F3;
            margin-bottom: 10px;
        }
        .status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-weight: bold;
            margin-left: 10px;
        }
        .status.loaded { background: #4caf50; color: white; }
        .status.not-loaded { background: #f44336; color: white; }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .card {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .card h2 {
            color: #2196F3;
            margin-bottom: 15px;
            font-size: 1.2em;
            border-bottom: 2px solid #2196F3;
            padding-bottom: 10px;
        }
        .metric {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #eee;
        }
        .metric:last-child { border-bottom: none; }
        .metric-label { color: #666; }
        .metric-value {
            font-weight: bold;
            color: #333;
        }
        .log-levels {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
        }
        .log-level {
            padding: 8px;
            border-radius: 5px;
            text-align: center;
            font-weight: bold;
        }
        .log-level.EMERG { background: #d32f2f; color: white; }
        .log-level.ALERT { background: #f44336; color: white; }
        .log-level.CRIT { background: #e91e63; color: white; }
        .log-level.ERR { background: #ff9800; color: white; }
        .log-level.WARNING { background: #ffc107; color: #333; }
        .log-level.NOTICE { background: #4caf50; color: white; }
        .log-level.INFO { background: #2196f3; color: white; }
        .log-level.DEBUG { background: #9e9e9e; color: white; }
        .logs-container {
            max-height: 400px;
            overflow-y: auto;
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 15px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
        }
        .log-entry {
            margin-bottom: 5px;
            padding: 5px;
            border-left: 3px solid #2196F3;
            padding-left: 10px;
        }
        .log-entry.EMERG { border-left-color: #d32f2f; }
        .log-entry.ALERT { border-left-color: #f44336; }
        .log-entry.CRIT { border-left-color: #e91e63; }
        .log-entry.ERR { border-left-color: #ff9800; }
        .log-entry.WARNING { border-left-color: #ffc107; }
        .log-entry.NOTICE { border-left-color: #4caf50; }
        .log-entry.INFO { border-left-color: #2196f3; }
        .log-entry.DEBUG { border-left-color: #9e9e9e; }
        .footer {
            text-align: center;
            color: white;
            margin-top: 20px;
            opacity: 0.8;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .updating {
            animation: pulse 2s infinite;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Kernel Module Debugging Dashboard
                <span id="module-status" class="status not-loaded">Module Not Loaded</span>
            </h1>
            <p>Real-time monitoring of debug_demo kernel module</p>
            <p style="margin-top: 10px; color: #666;">
                Last updated: <span id="last-update">-</span>
            </p>
        </div>

        <div class="grid">
            <div class="card">
                <h2>Module Status</h2>
                <div class="metric">
                    <span class="metric-label">Status:</span>
                    <span class="metric-value" id="status-value">Checking...</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Total Messages:</span>
                    <span class="metric-value" id="message-count">0</span>
                </div>
            </div>

            <div class="card">
                <h2>System Information</h2>
                <div class="metric">
                    <span class="metric-label">Kernel:</span>
                    <span class="metric-value" id="kernel-version">-</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Uptime:</span>
                    <span class="metric-value" id="uptime">-</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Load Average:</span>
                    <span class="metric-value" id="load-avg">-</span>
                </div>
            </div>

            <div class="card">
                <h2>Log Levels Distribution</h2>
                <div class="log-levels" id="log-levels">
                    <!-- Populated by JavaScript -->
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Recent Kernel Logs</h2>
            <div class="logs-container" id="logs-container">
                <div class="log-entry">Waiting for logs...</div>
            </div>
        </div>

        <div class="footer">
            <p>Kernel Module Debugging Techniques Dashboard | Auto-refreshing every 2 seconds</p>
        </div>
    </div>

    <script>
        function updateDashboard() {
            fetch('/api/metrics')
                .then(response => response.json())
                .then(data => {
                    // Update module status
                    const statusEl = document.getElementById('module-status');
                    const statusValue = document.getElementById('status-value');
                    if (data.module_loaded) {
                        statusEl.textContent = 'Module Loaded';
                        statusEl.className = 'status loaded';
                        statusValue.textContent = 'Loaded';
                    } else {
                        statusEl.textContent = 'Module Not Loaded';
                        statusEl.className = 'status not-loaded';
                        statusValue.textContent = 'Not Loaded';
                    }

                    // Update message count
                    document.getElementById('message-count').textContent = data.message_count;
                    document.getElementById('last-update').textContent = new Date(data.last_update).toLocaleString();

                    // Update system info
                    const sysInfo = data.system_info || {};
                    document.getElementById('kernel-version').textContent = 
                        sysInfo.kernel_version ? sysInfo.kernel_version.split(' ')[2] : '-';
                    document.getElementById('uptime').textContent = sysInfo.uptime || '-';
                    document.getElementById('load-avg').textContent = sysInfo.load_avg || '-';

                    // Update log levels
                    const levelsContainer = document.getElementById('log-levels');
                    levelsContainer.innerHTML = '';
                    for (const [level, count] of Object.entries(data.log_levels)) {
                        const div = document.createElement('div');
                        div.className = `log-level ${level}`;
                        div.textContent = `${level}: ${count}`;
                        levelsContainer.appendChild(div);
                    }

                    // Update logs
                    const logsContainer = document.getElementById('logs-container');
                    if (data.log_messages && data.log_messages.length > 0) {
                        logsContainer.innerHTML = '';
                        data.log_messages.slice(-20).reverse().forEach(log => {
                            const entry = document.createElement('div');
                            entry.className = 'log-entry';
                            const level = log.toUpperCase().includes('EMERG') ? 'EMERG' :
                                         log.toUpperCase().includes('ALERT') ? 'ALERT' :
                                         log.toUpperCase().includes('CRIT') ? 'CRIT' :
                                         log.toUpperCase().includes('ERR') ? 'ERR' :
                                         log.toUpperCase().includes('WARNING') ? 'WARNING' :
                                         log.toUpperCase().includes('NOTICE') ? 'NOTICE' :
                                         log.toUpperCase().includes('DEBUG') ? 'DEBUG' : 'INFO';
                            entry.className += ` ${level}`;
                            entry.textContent = log;
                            logsContainer.appendChild(entry);
                        });
                    } else {
                        logsContainer.innerHTML = '<div class="log-entry">No logs available. Load the module to see logs.</div>';
                    }
                })
                .catch(error => {
                    console.error('Error fetching metrics:', error);
                });
        }

        // Update immediately and then every 2 seconds
        updateDashboard();
        setInterval(updateDashboard, 2000);
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/metrics')
def get_metrics():
    # Convert deque to list for JSON serialization
    metrics_copy = dict(metrics)
    metrics_copy['log_messages'] = list(metrics['log_messages'])
    return jsonify(metrics_copy)

@app.route('/api/demo', methods=['POST'])
def enable_demo():
    """Enable demo mode and inject demo data"""
    global demo_mode, metrics
    data = request.get_json() or {}
    enable = data.get('enable', True)
    demo_mode = enable
    
    if enable:
        # Inject initial demo data
        demo_logs = [
            "[  123.456789] debug_demo: INFO: Module initialized successfully",
            "[  124.123456] debug_demo: DEBUG: Setting up device structures",
            "[  125.234567] debug_demo: NOTICE: Device registered with major number 247",
            "[  126.345678] debug_demo: INFO: Opening device file /dev/debug_demo",
            "[  127.456789] debug_demo: DEBUG: Read operation requested, buffer size: 1024",
            "[  128.567890] debug_demo: WARNING: Buffer size exceeds recommended limit",
            "[  129.678901] debug_demo: INFO: Write operation completed, bytes: 512",
            "[  130.789012] debug_demo: DEBUG: Processing ioctl command 0x1234",
            "[  131.890123] debug_demo: ERR: Invalid parameter detected in ioctl",
            "[  132.901234] debug_demo: INFO: Recovering from error state",
            "[  133.012345] debug_demo: NOTICE: Device operations resumed normally",
            "[  134.123456] debug_demo: DEBUG: Closing device file descriptor",
            "[  135.234567] debug_demo: INFO: Module cleanup initiated",
            "[  136.345678] debug_demo: DEBUG: Releasing allocated resources",
            "[  137.456789] debug_demo: NOTICE: Module unloaded successfully",
        ]
        
        # Clear existing metrics and add demo data
        metrics['log_messages'].clear()
        metrics['log_levels'] = {'EMERG': 0, 'ALERT': 0, 'CRIT': 0, 'ERR': 0, 'WARNING': 0, 'NOTICE': 0, 'INFO': 0, 'DEBUG': 0}
        metrics['message_count'] = 0
        
        for log in demo_logs:
            metrics['log_messages'].append(log)
            level = parse_log_level(log)
            metrics['log_levels'][level] = metrics['log_levels'].get(level, 0) + 1
            metrics['message_count'] += 1
        
        metrics['module_loaded'] = True
        metrics['last_update'] = datetime.now().isoformat()
        
        return jsonify({'status': 'success', 'message': 'Demo mode enabled', 'logs_added': len(demo_logs)})
    else:
        # Clear demo data
        metrics['log_messages'].clear()
        metrics['log_levels'] = {'EMERG': 0, 'ALERT': 0, 'CRIT': 0, 'ERR': 0, 'WARNING': 0, 'NOTICE': 0, 'INFO': 0, 'DEBUG': 0}
        metrics['message_count'] = 0
        metrics['module_loaded'] = check_module_loaded()
        return jsonify({'status': 'success', 'message': 'Demo mode disabled'})

if __name__ == '__main__':
    print("Starting Kernel Module Debugging Dashboard...")
    print("Dashboard available at: http://localhost:8080")
    print("Press Ctrl+C to stop")
    app.run(host='0.0.0.0', port=8080, debug=False)

