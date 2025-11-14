from flask import Flask, jsonify, request
from flask_cors import CORS
import requests
import time
import socket
import threading
from datetime import datetime

app = Flask(__name__)
CORS(app)

metrics = {
    'request_count': 0,
    'error_count': 0,
    'total_latency': 0,
    'dns_times': [],
    'active_threads': 0,
    'max_threads': 100
}
metrics_lock = threading.Lock()

def resolve_with_timing(hostname):
    """Resolve hostname and measure DNS time"""
    start = time.time()
    try:
        socket.gethostbyname(hostname)
        dns_time = (time.time() - start) * 1000
        return dns_time
    except:
        return 0

@app.route('/health')
def health():
    return jsonify({'status': 'healthy'})

@app.route('/api/request')
def handle_request():
    start_time = time.time()
    
    with metrics_lock:
        metrics['active_threads'] += 1
    
    try:
        # Measure DNS resolution time
        dns_time = resolve_with_timing('backend')
        
        # Call backend service
        response = requests.get('http://backend:8081/api/work', timeout=2)
        
        latency = (time.time() - start_time) * 1000
        
        with metrics_lock:
            metrics['request_count'] += 1
            metrics['total_latency'] += latency
            metrics['dns_times'].append(dns_time)
            if len(metrics['dns_times']) > 100:
                metrics['dns_times'] = metrics['dns_times'][-100:]
            metrics['active_threads'] -= 1
        
        return jsonify({
            'status': 'success',
            'latency_ms': round(latency, 2),
            'dns_ms': round(dns_time, 2),
            'backend_response': response.json()
        })
    
    except Exception as e:
        with metrics_lock:
            metrics['error_count'] += 1
            metrics['active_threads'] -= 1
        
        return jsonify({
            'status': 'error',
            'error': str(e)
        }), 500

@app.route('/metrics')
def get_metrics():
    with metrics_lock:
        avg_latency = metrics['total_latency'] / metrics['request_count'] if metrics['request_count'] > 0 else 0
        avg_dns = sum(metrics['dns_times']) / len(metrics['dns_times']) if metrics['dns_times'] else 0
        error_rate = (metrics['error_count'] / metrics['request_count'] * 100) if metrics['request_count'] > 0 else 0
        
        return jsonify({
            'service': 'frontend',
            'timestamp': datetime.now().isoformat(),
            'request_count': metrics['request_count'],
            'error_count': metrics['error_count'],
            'error_rate': round(error_rate, 2),
            'avg_latency_ms': round(avg_latency, 2),
            'avg_dns_ms': round(avg_dns, 2),
            'active_threads': metrics['active_threads'],
            'thread_utilization': round(metrics['active_threads'] / metrics['max_threads'] * 100, 2)
        })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, threaded=True)
