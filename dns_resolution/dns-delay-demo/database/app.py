from flask import Flask, jsonify
from flask_cors import CORS
import time
from datetime import datetime

app = Flask(__name__)
CORS(app)

metrics = {
    'request_count': 0,
    'total_latency': 0
}

@app.route('/health')
def health():
    return jsonify({'status': 'healthy'})

@app.route('/api/data')
def get_data():
    start_time = time.time()
    
    # Simulate database query
    time.sleep(0.005)
    
    latency = (time.time() - start_time) * 1000
    metrics['request_count'] += 1
    metrics['total_latency'] += latency
    
    return jsonify({
        'status': 'success',
        'data': {
            'id': metrics['request_count'],
            'value': 'sample_data'
        },
        'latency_ms': round(latency, 2)
    })

@app.route('/metrics')
def get_metrics():
    avg_latency = metrics['total_latency'] / metrics['request_count'] if metrics['request_count'] > 0 else 0
    
    return jsonify({
        'service': 'database',
        'timestamp': datetime.now().isoformat(),
        'request_count': metrics['request_count'],
        'avg_latency_ms': round(avg_latency, 2)
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8082, threaded=True)
