#!/usr/bin/env python3
"""
ARM Real-Time Latency Dashboard
Web dashboard for monitoring latency metrics and system status
"""

import os
import json
import time
import subprocess
import threading
from datetime import datetime
from pathlib import Path
from flask import Flask, render_template_string, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

SCRIPT_DIR = Path(__file__).parent
RESULTS_DIR = SCRIPT_DIR / "results"
BUILD_DIR = SCRIPT_DIR / "build"

# Dashboard HTML template
DASHBOARD_HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>ARM Real-Time Latency Dashboard</title>
    <meta http-equiv="refresh" content="5">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            padding: 20px;
        }
        .container {
            max-width: 1400px;
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
            border-bottom: 2px solid #667eea;
            padding-bottom: 10px;
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
            font-weight: bold;
            color: #333;
            font-size: 1.1em;
        }
        .status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-weight: bold;
            font-size: 0.9em;
        }
        .status.running {
            background: #4caf50;
            color: white;
        }
        .status.stopped {
            background: #f44336;
            color: white;
        }
        .status.unknown {
            background: #ff9800;
            color: white;
        }
        .chart-container {
            background: white;
            border-radius: 10px;
            padding: 20px;
            margin-top: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .timestamp {
            text-align: center;
            color: white;
            margin-top: 20px;
            font-size: 0.9em;
        }
        .error {
            color: #f44336;
            font-weight: bold;
        }
        .success {
            color: #4caf50;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 ARM Real-Time Latency Dashboard</h1>
        
        <div class="grid">
            <div class="card">
                <h2>System Status</h2>
                <div class="metric">
                    <span class="metric-label">Latency Test:</span>
                    <span class="metric-value">
                        <span class="status {{ latency_status }}">{{ latency_status_text }}</span>
                    </span>
                </div>
                <div class="metric">
                    <span class="metric-label">IRQ Monitor:</span>
                    <span class="metric-value">
                        <span class="status {{ irq_status }}">{{ irq_status_text }}</span>
                    </span>
                </div>
                <div class="metric">
                    <span class="metric-label">Cache Polluter:</span>
                    <span class="metric-value">
                        <span class="status {{ polluter_status }}">{{ polluter_status_text }}</span>
                    </span>
                </div>
                <div class="metric">
                    <span class="metric-label">Architecture:</span>
                    <span class="metric-value">{{ architecture }}</span>
                </div>
            </div>
            
            <div class="card">
                <h2>Latency Metrics</h2>
                <div class="metric">
                    <span class="metric-label">Min Latency:</span>
                    <span class="metric-value">{{ min_latency }}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Max Latency:</span>
                    <span class="metric-value">{{ max_latency }}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Total Samples:</span>
                    <span class="metric-value">{{ total_samples }}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Last Update:</span>
                    <span class="metric-value">{{ last_update }}</span>
                </div>
            </div>
            
            <div class="card">
                <h2>System Info</h2>
                <div class="metric">
                    <span class="metric-label">CPU Cores:</span>
                    <span class="metric-value">{{ cpu_cores }}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Load Average:</span>
                    <span class="metric-value">{{ load_avg }}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Memory Usage:</span>
                    <span class="metric-value">{{ memory_usage }}</span>
                </div>
            </div>
        </div>
        
        <div class="timestamp">
            Last updated: {{ current_time }}
        </div>
    </div>
</body>
</html>
"""

def get_process_status(process_name):
    """Check if a process is running"""
    try:
        result = subprocess.run(
            ['pgrep', '-f', process_name],
            capture_output=True,
            text=True,
            timeout=1
        )
        if result.returncode == 0:
            return 'running', 'Running'
        else:
            return 'stopped', 'Stopped'
    except:
        return 'unknown', 'Unknown'

def read_latency_stats():
    """Read latency statistics from file"""
    stats_file = RESULTS_DIR / "latency_stats.txt"
    stats = {
        'min_latency': 'N/A',
        'max_latency': 'N/A',
        'total_samples': 'N/A',
        'last_update': 'Never'
    }
    
    if stats_file.exists():
        try:
            with open(stats_file, 'r') as f:
                for line in f:
                    if 'Min:' in line:
                        stats['min_latency'] = line.split(':')[1].strip()
                    elif 'Max:' in line:
                        stats['max_latency'] = line.split(':')[1].strip()
                    elif 'Samples:' in line:
                        stats['total_samples'] = line.split(':')[1].strip()
            
            # Get file modification time
            mtime = stats_file.stat().st_mtime
            stats['last_update'] = datetime.fromtimestamp(mtime).strftime('%Y-%m-%d %H:%M:%S')
        except Exception as e:
            print(f"Error reading stats: {e}")
    
    return stats

def get_system_info():
    """Get system information"""
    info = {
        'architecture': 'Unknown',
        'cpu_cores': 'Unknown',
        'load_avg': 'Unknown',
        'memory_usage': 'Unknown'
    }
    
    try:
        # Architecture
        result = subprocess.run(['uname', '-m'], capture_output=True, text=True, timeout=1)
        if result.returncode == 0:
            info['architecture'] = result.stdout.strip()
        
        # CPU cores
        result = subprocess.run(['nproc'], capture_output=True, text=True, timeout=1)
        if result.returncode == 0:
            info['cpu_cores'] = result.stdout.strip()
        
        # Load average
        result = subprocess.run(['uptime'], capture_output=True, text=True, timeout=1)
        if result.returncode == 0:
            # Extract load average from uptime output
            parts = result.stdout.split('load average:')
            if len(parts) > 1:
                info['load_avg'] = parts[1].strip().split(',')[0].strip()
        
        # Memory usage (simplified)
        try:
            with open('/proc/meminfo', 'r') as f:
                meminfo = f.read()
                for line in meminfo.split('\n'):
                    if 'MemTotal:' in line:
                        total = int(line.split()[1]) // 1024  # Convert to MB
                    elif 'MemAvailable:' in line:
                        available = int(line.split()[1]) // 1024
                        used = total - available
                        info['memory_usage'] = f"{used}MB / {total}MB ({used*100//total}%)"
                        break
        except:
            pass
            
    except Exception as e:
        print(f"Error getting system info: {e}")
    
    return info

@app.route('/')
def dashboard():
    """Main dashboard page"""
    # Get process statuses
    latency_status, latency_text = get_process_status('latency_test')
    irq_status, irq_text = get_process_status('irq_monitor')
    polluter_status, polluter_text = get_process_status('cache_polluter')
    
    # Get latency stats
    latency_stats = read_latency_stats()
    
    # Get system info
    system_info = get_system_info()
    
    # Render template
    return render_template_string(DASHBOARD_HTML,
        latency_status=latency_status,
        latency_status_text=latency_text,
        irq_status=irq_status,
        irq_status_text=irq_text,
        polluter_status=polluter_status,
        polluter_status_text=polluter_text,
        min_latency=latency_stats['min_latency'],
        max_latency=latency_stats['max_latency'],
        total_samples=latency_stats['total_samples'],
        last_update=latency_stats['last_update'],
        architecture=system_info['architecture'],
        cpu_cores=system_info['cpu_cores'],
        load_avg=system_info['load_avg'],
        memory_usage=system_info['memory_usage'],
        current_time=datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    )

@app.route('/api/metrics')
def api_metrics():
    """API endpoint for metrics"""
    latency_stats = read_latency_stats()
    system_info = get_system_info()
    
    return jsonify({
        'latency': latency_stats,
        'system': system_info,
        'processes': {
            'latency_test': get_process_status('latency_test')[1],
            'irq_monitor': get_process_status('irq_monitor')[1],
            'cache_polluter': get_process_status('cache_polluter')[1]
        },
        'timestamp': datetime.now().isoformat()
    })

if __name__ == '__main__':
    PORT = 8080
    print("Starting ARM Real-Time Latency Dashboard...")
    print(f"Dashboard available at: http://localhost:{PORT}")
    print(f"API endpoint: http://localhost:{PORT}/api/metrics")
    app.run(host='0.0.0.0', port=PORT, debug=False)

