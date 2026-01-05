#!/usr/bin/env python3
"""
Dashboard for syscall monitoring metrics
Displays real-time statistics from strace output
"""

import os
import sys
import json
import time
import re
import subprocess
from pathlib import Path
from collections import defaultdict
from datetime import datetime
from flask import Flask, render_template_string, jsonify
import threading

app = Flask(__name__)

SCRIPT_DIR = Path(__file__).parent.absolute()
LOGS_DIR = SCRIPT_DIR / "logs"
STRACE_LOG = LOGS_DIR / "strace_output.log"

# Global metrics storage
metrics = {
    'total_calls': 0,
    'total_failures': 0,
    'syscalls': defaultdict(lambda: {'count': 0, 'failed': 0, 'total_time': 0.0}),
    'last_update': None,
    'error_types': defaultdict(int),
    'timeline': []
}

metrics_lock = threading.Lock()

def parse_strace_line(line):
    """Parse a line from strace output and extract metrics"""
    if not line.strip():
        return None
    
    # Extract syscall name
    syscall_match = re.search(r'(\w+)\(', line)
    if not syscall_match:
        return None
    
    syscall_name = syscall_match.group(1)
    
    # Check for failure
    is_failure = bool(re.search(r'= -1|ENOENT|EACCES|EPERM|ECONNREFUSED|EMFILE|EBADF', line))
    
    # Extract timing if present <0.000123>
    time_match = re.search(r'<([\d.]+)>', line)
    elapsed = float(time_match.group(1)) if time_match else 0.0
    
    # Extract error type
    error_type = None
    if is_failure:
        error_match = re.search(r'(ENOENT|EACCES|EPERM|ECONNREFUSED|EMFILE|EBADF|EINVAL)', line)
        if error_match:
            error_type = error_match.group(1)
    
    return {
        'syscall': syscall_name,
        'failed': is_failure,
        'time': elapsed,
        'error': error_type
    }

def update_metrics():
    """Read strace log and update metrics"""
    global metrics
    
    if not STRACE_LOG.exists():
        return
    
    try:
        with open(STRACE_LOG, 'r') as f:
            lines = f.readlines()
        
        with metrics_lock:
            # Reset if file was truncated
            if len(lines) < metrics['total_calls']:
                metrics['total_calls'] = 0
                metrics['total_failures'] = 0
                metrics['syscalls'] = defaultdict(lambda: {'count': 0, 'failed': 0, 'total_time': 0.0})
                metrics['error_types'] = defaultdict(int)
            
            # Process new lines
            for line in lines[metrics['total_calls']:]:
                parsed = parse_strace_line(line)
                if parsed:
                    metrics['total_calls'] += 1
                    syscall = parsed['syscall']
                    
                    metrics['syscalls'][syscall]['count'] += 1
                    metrics['syscalls'][syscall]['total_time'] += parsed['time']
                    
                    if parsed['failed']:
                        metrics['total_failures'] += 1
                        metrics['syscalls'][syscall]['failed'] += 1
                        if parsed['error']:
                            metrics['error_types'][parsed['error']] += 1
                    
                    # Add to timeline (keep last 100 entries)
                    metrics['timeline'].append({
                        'time': datetime.now().isoformat(),
                        'syscall': syscall,
                        'failed': parsed['failed']
                    })
                    if len(metrics['timeline']) > 100:
                        metrics['timeline'].pop(0)
            
            metrics['last_update'] = datetime.now().isoformat()
    except Exception as e:
        print(f"Error updating metrics: {e}", file=sys.stderr)

def background_metrics_updater():
    """Background thread to update metrics periodically"""
    while True:
        update_metrics()
        time.sleep(1)  # Update every second

# HTML template for dashboard
DASHBOARD_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Syscall Monitor Dashboard</title>
    <meta http-equiv="refresh" content="2">
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: #1e1e1e;
            color: #d4d4d4;
        }
        .header {
            background: #2d2d30;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        h1 {
            margin: 0;
            color: #4ec9b0;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .stat-card {
            background: #252526;
            padding: 20px;
            border-radius: 8px;
            border-left: 4px solid #4ec9b0;
        }
        .stat-value {
            font-size: 2em;
            font-weight: bold;
            color: #4ec9b0;
        }
        .stat-label {
            color: #858585;
            margin-top: 5px;
        }
        .failure-rate {
            color: #f48771;
        }
        .success-rate {
            color: #4ec9b0;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background: #252526;
            border-radius: 8px;
            overflow: hidden;
        }
        th {
            background: #2d2d30;
            padding: 12px;
            text-align: left;
            color: #4ec9b0;
        }
        td {
            padding: 10px 12px;
            border-top: 1px solid #3e3e42;
        }
        tr:hover {
            background: #2d2d30;
        }
        .failed {
            color: #f48771;
        }
        .success {
            color: #4ec9b0;
        }
        .status {
            display: inline-block;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 0.8em;
        }
        .status.active {
            background: #4ec9b0;
            color: #1e1e1e;
        }
        .status.inactive {
            background: #858585;
            color: #1e1e1e;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🔍 Syscall Monitor Dashboard</h1>
        <p>Real-time system call monitoring and analysis</p>
        <p>Last Update: {{ last_update or 'Never' }}</p>
    </div>
    
    <div class="stats-grid">
        <div class="stat-card">
            <div class="stat-value">{{ total_calls }}</div>
            <div class="stat-label">Total Syscalls</div>
        </div>
        <div class="stat-card">
            <div class="stat-value failure-rate">{{ total_failures }}</div>
            <div class="stat-label">Failed Syscalls</div>
        </div>
        <div class="stat-card">
            <div class="stat-value failure-rate">{{ failure_rate }}%</div>
            <div class="stat-label">Failure Rate</div>
        </div>
        <div class="stat-card">
            <div class="stat-value success-rate">{{ success_rate }}%</div>
            <div class="stat-label">Success Rate</div>
        </div>
    </div>
    
    <h2>Syscall Statistics</h2>
    <table>
        <thead>
            <tr>
                <th>Syscall</th>
                <th>Total Calls</th>
                <th>Failed</th>
                <th>Success</th>
                <th>Failure Rate</th>
                <th>Avg Time (ms)</th>
            </tr>
        </thead>
        <tbody>
            {% for syscall, data in syscalls.items() %}
            <tr>
                <td><strong>{{ syscall }}</strong></td>
                <td>{{ data.count }}</td>
                <td class="failed">{{ data.failed }}</td>
                <td class="success">{{ data.count - data.failed }}</td>
                <td class="failed">{{ "%.1f"|format((data.failed / data.count * 100) if data.count > 0 else 0) }}%</td>
                <td>{{ "%.3f"|format((data.total_time / data.count * 1000) if data.count > 0 else 0) }}</td>
            </tr>
            {% endfor %}
        </tbody>
    </table>
    
    <h2>Error Types</h2>
    <table>
        <thead>
            <tr>
                <th>Error Code</th>
                <th>Count</th>
                <th>Description</th>
            </tr>
        </thead>
        <tbody>
            {% for error, count in error_types.items() %}
            <tr>
                <td class="failed"><strong>{{ error }}</strong></td>
                <td>{{ count }}</td>
                <td>{{ error_descriptions.get(error, 'Unknown error') }}</td>
            </tr>
            {% endfor %}
        </tbody>
    </table>
</body>
</html>
"""

ERROR_DESCRIPTIONS = {
    'ENOENT': 'No such file or directory',
    'EACCES': 'Permission denied',
    'EPERM': 'Operation not permitted',
    'ECONNREFUSED': 'Connection refused',
    'EMFILE': 'Too many open files',
    'EBADF': 'Bad file descriptor',
    'EINVAL': 'Invalid argument'
}

@app.route('/')
def dashboard():
    """Main dashboard page"""
    with metrics_lock:
        failure_rate = (metrics['total_failures'] / metrics['total_calls'] * 100) if metrics['total_calls'] > 0 else 0
        success_rate = 100 - failure_rate
        
        # Sort syscalls by count
        sorted_syscalls = sorted(metrics['syscalls'].items(), key=lambda x: x[1]['count'], reverse=True)
        
        return render_template_string(
            DASHBOARD_TEMPLATE,
            total_calls=metrics['total_calls'],
            total_failures=metrics['total_failures'],
            failure_rate=f"{failure_rate:.1f}",
            success_rate=f"{success_rate:.1f}",
            syscalls=dict(sorted_syscalls),
            error_types=dict(metrics['error_types']),
            error_descriptions=ERROR_DESCRIPTIONS,
            last_update=metrics['last_update']
        )

@app.route('/api/metrics')
def api_metrics():
    """JSON API for metrics"""
    with metrics_lock:
        failure_rate = (metrics['total_failures'] / metrics['total_calls'] * 100) if metrics['total_calls'] > 0 else 0
        return jsonify({
            'total_calls': metrics['total_calls'],
            'total_failures': metrics['total_failures'],
            'failure_rate': failure_rate,
            'success_rate': 100 - failure_rate,
            'syscalls': dict(metrics['syscalls']),
            'error_types': dict(metrics['error_types']),
            'last_update': metrics['last_update'],
            'timeline': metrics['timeline'][-20:]  # Last 20 entries
        })

@app.route('/api/health')
def health():
    """Health check endpoint"""
    monitor_running = subprocess.run(['pgrep', '-f', 'syscall_monitor'], 
                                     capture_output=True).returncode == 0
    return jsonify({
        'status': 'healthy' if monitor_running else 'monitor_not_running',
        'monitor_running': monitor_running,
        'timestamp': datetime.now().isoformat()
    })

if __name__ == '__main__':
    # Ensure logs directory exists
    LOGS_DIR.mkdir(exist_ok=True)
    
    # Start background metrics updater
    updater_thread = threading.Thread(target=background_metrics_updater, daemon=True)
    updater_thread.start()
    
    print("Starting dashboard server on http://localhost:5000")
    print("Press Ctrl+C to stop")
    app.run(host='0.0.0.0', port=5000, debug=False)

