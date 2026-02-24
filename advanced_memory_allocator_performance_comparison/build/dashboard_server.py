#!/usr/bin/env python3
# dashboard_server.py — Simple HTTP server for dashboard and API
import http.server
import socketserver
import json
import os
import re
import subprocess
from pathlib import Path
from urllib.parse import urlparse, parse_qs

PORT = 8080
WORKDIR = Path(__file__).parent.parent
RESULTS_DIR = WORKDIR / "results"
RESULTS_DIR.mkdir(exist_ok=True)

class DashboardHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/dashboard.html':
            self.path = '/dashboard.html'
            return super().do_GET()
        elif self.path == '/api/results':
            self.send_json_response(self.get_results())
        else:
            super().do_GET()

    def do_POST(self):
        if self.path == '/api/run':
            result = self.run_benchmark()
            self.send_json_response(result)
        else:
            self.send_error(404)

    def send_json_response(self, data):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def get_results(self):
        results = {
            'ptmalloc2': {'throughput': 0, 'p99': 0, 'rss': 0},
            'tcmalloc': {'throughput': 0, 'p99': 0, 'rss': 0},
            'jemalloc': {'throughput': 0, 'p99': 0, 'rss': 0}
        }
        
        for alloc in ['ptmalloc2', 'tcmalloc', 'jemalloc']:
            result_file = RESULTS_DIR / f"{alloc}.txt"
            if result_file.exists():
                content = result_file.read_text()
                # Parse throughput
                m = re.search(r'Throughput:\s+(\d+(?:\.\d+)?)', content)
                if m:
                    results[alloc]['throughput'] = float(m.group(1))
                # Parse p99
                m = re.search(r'p99:\s+(\d+(?:\.\d+)?)', content)
                if m:
                    results[alloc]['p99'] = float(m.group(1))
                # Parse RSS delta
                m = re.search(r'RSS delta:\s+([+-]?\d+)', content)
                if m:
                    results[alloc]['rss'] = int(m.group(1))
        
        return results

    def run_benchmark(self):
        try:
            startup_script = WORKDIR / "build" / "startup.sh"
            if startup_script.exists():
                subprocess.Popen(['bash', str(startup_script)], cwd=str(WORKDIR))
                return {'message': 'Benchmark started', 'status': 'success'}
            else:
                return {'message': 'Startup script not found', 'status': 'error'}
        except Exception as e:
            return {'message': str(e), 'status': 'error'}

    def log_message(self, format, *args):
        return  # Suppress default logging

if __name__ == "__main__":
    os.chdir(Path(__file__).parent)
    class ReusableTCPServer(socketserver.TCPServer):
        allow_reuse_address = True
    with ReusableTCPServer(("", PORT), DashboardHandler) as httpd:
        print(f"Dashboard server running on http://localhost:{PORT}")
        print("Press Ctrl+C to stop")
        httpd.serve_forever()
