#!/usr/bin/env python3
"""
Performance Metrics Dashboard for Dynamic Linking Analysis
Displays real-time metrics comparing lazy vs immediate binding
"""

import subprocess
import json
import time
import os
import sys
from pathlib import Path
from flask import Flask, render_template, jsonify
from threading import Thread
import re

app = Flask(__name__)

BASE_DIR = Path(__file__).parent
BUILD_DIR = BASE_DIR / "build"
METRICS_FILE = BASE_DIR / "metrics.json"

# Global metrics storage
metrics = {
    "lazy": {
        "startup_time": 0.0,
        "first_call_cycles": 0,
        "second_call_cycles": 0,
        "overhead_cycles": 0,
        "overhead_percent": 0.0,
        "last_update": None
    },
    "immediate": {
        "startup_time": 0.0,
        "first_call_cycles": 0,
        "second_call_cycles": 0,
        "overhead_cycles": 0,
        "overhead_percent": 0.0,
        "last_update": None
    },
    "system_info": {
        "hostname": "",
        "timestamp": None
    }
}

def parse_binding_test_output(output):
    """Parse output from binding_test binary"""
    result = {
        "first_call_cycles": 0,
        "second_call_cycles": 0,
        "overhead_cycles": 0,
        "overhead_percent": 0.0
    }
    
    # Extract cycle counts
    first_match = re.search(r'First call:\s+(\d+)', output)
    second_match = re.search(r'Second call:\s+(\d+)', output)
    overhead_match = re.search(r'PLT resolution overhead:\s+(\d+)\s+cycles\s+\(([\d.]+)%\)', output)
    
    if first_match:
        result["first_call_cycles"] = int(first_match.group(1))
    if second_match:
        result["second_call_cycles"] = int(second_match.group(1))
    if overhead_match:
        result["overhead_cycles"] = int(overhead_match.group(1))
        result["overhead_percent"] = float(overhead_match.group(2))
    
    return result

def measure_startup_time(binary_path):
    """Measure startup time using /usr/bin/time"""
    try:
        result = subprocess.run(
            ['/usr/bin/time', '-f', '%e', binary_path],
            capture_output=True,
            text=True,
            timeout=5,
            cwd=str(BASE_DIR)
        )
        # Parse elapsed time from stderr
        lines = result.stderr.strip().split('\n')
        for line in lines:
            try:
                return float(line.strip())
            except ValueError:
                continue
        return 0.0
    except Exception as e:
        print(f"Error measuring startup time: {e}", file=sys.stderr)
        return 0.0

def collect_metrics():
    """Collect performance metrics from binaries"""
    lazy_binary = BUILD_DIR / "binding_test_lazy"
    now_binary = BUILD_DIR / "binding_test_now"
    
    if not lazy_binary.exists() or not now_binary.exists():
        print(f"Error: Binaries not found in {BUILD_DIR}", file=sys.stderr)
        return
    
    # Measure lazy binding
    try:
        result = subprocess.run(
            [str(lazy_binary)],
            capture_output=True,
            text=True,
            timeout=5,
            cwd=str(BASE_DIR)
        )
        lazy_data = parse_binding_test_output(result.stdout)
        lazy_data["startup_time"] = measure_startup_time(str(lazy_binary))
        lazy_data["last_update"] = time.time()
        metrics["lazy"].update(lazy_data)
    except Exception as e:
        print(f"Error collecting lazy metrics: {e}", file=sys.stderr)
    
    # Measure immediate binding
    try:
        result = subprocess.run(
            [str(now_binary)],
            capture_output=True,
            text=True,
            timeout=5,
            cwd=str(BASE_DIR)
        )
        now_data = parse_binding_test_output(result.stdout)
        now_data["startup_time"] = measure_startup_time(str(now_binary))
        now_data["last_update"] = time.time()
        metrics["immediate"].update(now_data)
    except Exception as e:
        print(f"Error collecting immediate metrics: {e}", file=sys.stderr)
    
    # Update system info
    metrics["system_info"]["timestamp"] = time.time()
    metrics["system_info"]["hostname"] = os.uname().nodename
    
    # Save to file
    try:
        with open(METRICS_FILE, 'w') as f:
            json.dump(metrics, f, indent=2)
    except Exception as e:
        print(f"Error saving metrics: {e}", file=sys.stderr)

def metrics_collector_thread():
    """Background thread to collect metrics periodically"""
    while True:
        collect_metrics()
        time.sleep(10)  # Update every 10 seconds

@app.route('/')
def index():
    """Main dashboard page"""
    return render_template('dashboard.html')

@app.route('/api/metrics')
def get_metrics():
    """API endpoint for metrics"""
    return jsonify(metrics)

@app.route('/api/collect')
def trigger_collect():
    """Manually trigger metrics collection"""
    collect_metrics()
    return jsonify({"status": "success", "message": "Metrics collected"})

@app.route('/favicon.ico')
def favicon():
    """Handle favicon requests to prevent 404 errors"""
    from flask import Response
    return Response(status=204)  # No Content

if __name__ == '__main__':
    # Create templates directory if it doesn't exist
    templates_dir = BASE_DIR / "templates"
    templates_dir.mkdir(exist_ok=True)
    
    # Initial metrics collection
    collect_metrics()
    
    # Start background metrics collector
    collector = Thread(target=metrics_collector_thread, daemon=True)
    collector.start()
    
    print("Starting dashboard server on http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=False)

