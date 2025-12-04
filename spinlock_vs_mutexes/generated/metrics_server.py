#!/usr/bin/env python3
import json
import re
import os
import subprocess
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import threading

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_DIR = os.path.join(SCRIPT_DIR, 'logs')

class MetricsHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/dashboard.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            with open(os.path.join(SCRIPT_DIR, 'dashboard.html'), 'rb') as f:
                self.wfile.write(f.read())
        elif self.path == '/api/metrics':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            metrics = self.get_metrics()
            self.wfile.write(json.dumps(metrics).encode())
        elif self.path == '/favicon.ico':
            # Return 204 No Content to stop browser from requesting favicon
            self.send_response(204)
            self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path == '/api/start':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            result = self.start_demo()
            self.wfile.write(json.dumps(result).encode())
        elif self.path == '/api/stop':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            result = self.stop_demo()
            self.wfile.write(json.dumps(result).encode())
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        pass  # Suppress default logging

    def get_metrics(self):
        metrics = {'spinlock': None, 'mutex': None}
        
        # Read spinlock log
        spinlock_log = os.path.join(LOG_DIR, 'spinlock.log')
        if os.path.exists(spinlock_log):
            with open(spinlock_log, 'r') as f:
                content = f.read()
                ops_match = re.search(r'Operations/second:\s+([\d.]+)', content)
                lat_match = re.search(r'Avg latency:\s+([\d.]+)\s+ns', content)
                if ops_match and lat_match:
                    metrics['spinlock'] = {
                        'ops_per_sec': float(ops_match.group(1)),
                        'avg_latency_ns': float(lat_match.group(1))
                    }
        
        # Read mutex log
        mutex_log = os.path.join(LOG_DIR, 'mutex.log')
        if os.path.exists(mutex_log):
            with open(mutex_log, 'r') as f:
                content = f.read()
                ops_match = re.search(r'Operations/second:\s+([\d.]+)', content)
                lat_match = re.search(r'Avg latency:\s+([\d.]+)\s+ns', content)
                if ops_match and lat_match:
                    metrics['mutex'] = {
                        'ops_per_sec': float(ops_match.group(1)),
                        'avg_latency_ns': float(lat_match.group(1))
                    }
        
        return metrics

    def start_demo(self):
        try:
            start_script = os.path.join(SCRIPT_DIR, 'start.sh')
            subprocess.Popen(['bash', start_script], cwd=SCRIPT_DIR)
            time.sleep(2)  # Give it time to start
            return {'success': True}
        except Exception as e:
            return {'success': False, 'error': str(e)}

    def stop_demo(self):
        try:
            stop_script = os.path.join(SCRIPT_DIR, 'stop.sh')
            subprocess.Popen(['bash', stop_script], cwd=SCRIPT_DIR)
            return {'success': True}
        except Exception as e:
            return {'success': False, 'error': str(e)}

def run_server(port=8080):
    server = HTTPServer(('', port), MetricsHandler)
    print(f"Metrics server running on http://localhost:{port}")
    print(f"Dashboard available at http://localhost:{port}/dashboard.html")
    server.serve_forever()

if __name__ == '__main__':
    run_server()
