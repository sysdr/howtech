#!/bin/bash

set -e

echo "🚀 DNS Resolution Delay Demo Setup"
echo "===================================="
echo ""

# Check prerequisites
echo "📋 Checking prerequisites..."
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed. Please install Docker first."
    exit 1
fi

if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
    echo "❌ Docker Compose is not installed. Please install Docker Compose first."
    exit 1
fi

echo "✅ Docker and Docker Compose are installed"
echo ""

# Create project structure
echo "📁 Creating project structure..."
mkdir -p dns-delay-demo/{frontend,backend,database,dns-server,dashboard,loadgen}
cd dns-delay-demo

# Create Frontend Service
echo "🔨 Creating Frontend service..."
cat > frontend/app.py << 'EOF'
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
EOF

# Create Backend Service
echo "🔨 Creating Backend service..."
cat > backend/app.py << 'EOF'
from flask import Flask, jsonify
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

@app.route('/api/work')
def do_work():
    start_time = time.time()
    
    with metrics_lock:
        metrics['active_threads'] += 1
    
    try:
        # Measure DNS resolution time
        dns_time = resolve_with_timing('database')
        
        # Call database service
        response = requests.get('http://database:8082/api/data', timeout=2)
        
        # Simulate some processing
        time.sleep(0.01)
        
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
            'data': response.json()
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
            'service': 'backend',
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
    app.run(host='0.0.0.0', port=8081, threaded=True)
EOF

# Create Database Service
echo "🔨 Creating Database service..."
cat > database/app.py << 'EOF'
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
EOF

# Create DNS Server
echo "🔨 Creating DNS server..."
cat > dns-server/dns_server.py << 'EOF'
from dnslib import DNSRecord, RR, QTYPE, A
from dnslib.server import DNSServer, DNSHandler
import time
import threading
from flask import Flask, jsonify, request
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# DNS delay configuration (in seconds)
dns_delay = {'value': 0.0}
delay_lock = threading.Lock()

class DelayDNSHandler(DNSHandler):
    def handle(self):
        with delay_lock:
            delay = dns_delay['value']
        
        if delay > 0:
            time.sleep(delay)
        
        super().handle()

def resolve(request, handler):
    """Resolve DNS queries"""
    reply = request.reply()
    qname = request.q.qname
    qtype = request.q.qtype
    
    # Map service names to IPs
    mappings = {
        'frontend': '172.20.0.10',
        'backend': '172.20.0.11',
        'database': '172.20.0.12'
    }
    
    hostname = str(qname).rstrip('.')
    
    if qtype == QTYPE.A:
        for service, ip in mappings.items():
            if service in hostname:
                reply.add_answer(RR(qname, QTYPE.A, rdata=A(ip), ttl=60))
                break
    
    return reply

@app.route('/health')
def health():
    return jsonify({'status': 'healthy'})

@app.route('/control/delay', methods=['POST'])
def set_delay():
    data = request.json
    delay_ms = data.get('delay_ms', 0)
    
    with delay_lock:
        dns_delay['value'] = delay_ms / 1000.0
    
    return jsonify({
        'status': 'success',
        'delay_ms': delay_ms
    })

@app.route('/control/delay', methods=['GET'])
def get_delay():
    with delay_lock:
        delay_ms = dns_delay['value'] * 1000
    
    return jsonify({
        'delay_ms': delay_ms
    })

def run_flask():
    app.run(host='0.0.0.0', port=5380, threaded=True)

if __name__ == '__main__':
    # Start Flask control server in background
    flask_thread = threading.Thread(target=run_flask, daemon=True)
    flask_thread.start()
    
    # Start DNS server
    dns = DNSServer(resolve, port=53, address='0.0.0.0', tcp=False, handler=DelayDNSHandler)
    print("DNS Server started on port 53 with control API on port 5380")
    dns.start_thread()
    
    # Keep main thread alive
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
EOF

# Create Load Generator
echo "🔨 Creating load generator..."
cat > loadgen/loadgen.py << 'EOF'
import requests
import time
import threading
from datetime import datetime

target_url = 'http://frontend:8080/api/request'
requests_per_second = 10
running = True

def make_request():
    try:
        response = requests.get(target_url, timeout=5)
        status = 'success' if response.status_code == 200 else 'error'
        print(f"[{datetime.now().strftime('%H:%M:%S')}] Request: {status}")
    except Exception as e:
        print(f"[{datetime.now().strftime('%H:%M:%S')}] Request: error - {str(e)[:50]}")

def load_generator():
    while running:
        make_request()
        time.sleep(1.0 / requests_per_second)

if __name__ == '__main__':
    print(f"Starting load generator: {requests_per_second} req/s to {target_url}")
    
    # Start multiple threads
    threads = []
    for i in range(5):
        t = threading.Thread(target=load_generator, daemon=True)
        t.start()
        threads.append(t)
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        running = False
        print("\nStopping load generator...")
EOF

# Create Dashboard
echo "🔨 Creating dashboard..."
cat > dashboard/index.html << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DNS Resolution Delay Monitor</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
        }
        
        .header {
            background: white;
            border-radius: 16px;
            padding: 30px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.1);
            margin-bottom: 30px;
        }
        
        h1 {
            color: #1a202c;
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        .subtitle {
            color: #718096;
            font-size: 1.1em;
        }
        
        .controls {
            background: white;
            border-radius: 16px;
            padding: 30px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.1);
            margin-bottom: 30px;
        }
        
        .control-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            font-weight: 600;
            color: #2d3748;
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #e2e8f0;
            outline: none;
            -webkit-appearance: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
        
        .delay-value {
            display: inline-block;
            background: #667eea;
            color: white;
            padding: 8px 20px;
            border-radius: 8px;
            font-weight: 700;
            font-size: 1.2em;
            margin-top: 10px;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .stat-card {
            background: white;
            border-radius: 16px;
            padding: 25px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.1);
            border-left: 5px solid #667eea;
        }
        
        .stat-card.warning {
            border-left-color: #f6ad55;
        }
        
        .stat-card.danger {
            border-left-color: #fc8181;
        }
        
        .stat-label {
            color: #718096;
            font-size: 0.9em;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 8px;
        }
        
        .stat-value {
            color: #1a202c;
            font-size: 2.5em;
            font-weight: 700;
        }
        
        .stat-unit {
            color: #718096;
            font-size: 0.6em;
            margin-left: 5px;
        }
        
        .charts-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(500px, 1fr));
            gap: 30px;
            margin-bottom: 30px;
        }
        
        .chart-card {
            background: white;
            border-radius: 16px;
            padding: 25px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.1);
        }
        
        .chart-title {
            color: #2d3748;
            font-size: 1.3em;
            font-weight: 600;
            margin-bottom: 20px;
        }
        
        .service-metrics {
            background: white;
            border-radius: 16px;
            padding: 30px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.1);
        }
        
        .service-row {
            display: grid;
            grid-template-columns: 150px repeat(4, 1fr);
            gap: 20px;
            padding: 15px;
            border-bottom: 1px solid #e2e8f0;
            align-items: center;
        }
        
        .service-row:last-child {
            border-bottom: none;
        }
        
        .service-name {
            font-weight: 700;
            color: #2d3748;
            font-size: 1.1em;
        }
        
        .metric {
            text-align: center;
        }
        
        .metric-label {
            font-size: 0.85em;
            color: #718096;
            margin-bottom: 5px;
        }
        
        .metric-value {
            font-size: 1.3em;
            font-weight: 700;
            color: #2d3748;
        }
        
        @media (max-width: 768px) {
            .charts-grid {
                grid-template-columns: 1fr;
            }
            
            .service-row {
                grid-template-columns: 1fr;
                text-align: center;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔍 DNS Resolution Delay Monitor</h1>
            <p class="subtitle">Watch how DNS delays cascade through your microservices in real-time</p>
        </div>
        
        <div class="controls">
            <div class="control-group">
                <label for="dnsDelay">DNS Delay Injection (milliseconds)</label>
                <input type="range" id="dnsDelay" min="0" max="1000" value="0" step="50">
                <div class="delay-value">Current Delay: <span id="delayValue">0</span> ms</div>
            </div>
        </div>
        
        <div class="stats-grid">
            <div class="stat-card" id="latencyCard">
                <div class="stat-label">Average Latency</div>
                <div class="stat-value"><span id="avgLatency">0</span><span class="stat-unit">ms</span></div>
            </div>
            <div class="stat-card" id="dnsCard">
                <div class="stat-label">DNS Resolution Time</div>
                <div class="stat-value"><span id="dnsTime">0</span><span class="stat-unit">ms</span></div>
            </div>
            <div class="stat-card" id="threadCard">
                <div class="stat-label">Thread Utilization</div>
                <div class="stat-value"><span id="threadUtil">0</span><span class="stat-unit">%</span></div>
            </div>
            <div class="stat-card" id="errorCard">
                <div class="stat-label">Error Rate</div>
                <div class="stat-value"><span id="errorRate">0</span><span class="stat-unit">%</span></div>
            </div>
        </div>
        
        <div class="charts-grid">
            <div class="chart-card">
                <div class="chart-title">Request Latency Over Time</div>
                <canvas id="latencyChart"></canvas>
            </div>
            <div class="chart-card">
                <div class="chart-title">Thread Pool Utilization</div>
                <canvas id="threadChart"></canvas>
            </div>
        </div>
        
        <div class="service-metrics">
            <h2 style="margin-bottom: 20px; color: #2d3748;">Service Metrics</h2>
            <div class="service-row" style="background: #f7fafc; font-weight: 600;">
                <div class="service-name">Service</div>
                <div class="metric">Requests</div>
                <div class="metric">Avg Latency</div>
                <div class="metric">DNS Time</div>
                <div class="metric">Errors</div>
            </div>
            <div class="service-row">
                <div class="service-name">Frontend</div>
                <div class="metric"><span id="frontendReqs">0</span></div>
                <div class="metric"><span id="frontendLatency">0</span> ms</div>
                <div class="metric"><span id="frontendDns">0</span> ms</div>
                <div class="metric"><span id="frontendErrors">0</span>%</div>
            </div>
            <div class="service-row">
                <div class="service-name">Backend</div>
                <div class="metric"><span id="backendReqs">0</span></div>
                <div class="metric"><span id="backendLatency">0</span> ms</div>
                <div class="metric"><span id="backendDns">0</span> ms</div>
                <div class="metric"><span id="backendErrors">0</span>%</div>
            </div>
            <div class="service-row">
                <div class="service-name">Database</div>
                <div class="metric"><span id="databaseReqs">0</span></div>
                <div class="metric"><span id="databaseLatency">0</span> ms</div>
                <div class="metric">N/A</div>
                <div class="metric">0%</div>
            </div>
        </div>
    </div>
    
    <script>
        // Chart setup
        const latencyCtx = document.getElementById('latencyChart').getContext('2d');
        const threadCtx = document.getElementById('threadChart').getContext('2d');
        
        const latencyChart = new Chart(latencyCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Latency (ms)',
                    data: [],
                    borderColor: '#667eea',
                    backgroundColor: 'rgba(102, 126, 234, 0.1)',
                    tension: 0.4,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    }
                }
            }
        });
        
        const threadChart = new Chart(threadCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Thread Utilization (%)',
                    data: [],
                    borderColor: '#f6ad55',
                    backgroundColor: 'rgba(246, 173, 85, 0.1)',
                    tension: 0.4,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        max: 100
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    }
                }
            }
        });
        
        // DNS delay control
        const dnsDelaySlider = document.getElementById('dnsDelay');
        const delayValue = document.getElementById('delayValue');
        
        dnsDelaySlider.addEventListener('input', async (e) => {
            const delay = e.target.value;
            delayValue.textContent = delay;
            
            try {
                await fetch('http://localhost:5380/control/delay', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({delay_ms: parseInt(delay)})
                });
            } catch (err) {
                console.error('Failed to set DNS delay:', err);
            }
        });
        
        // Update metrics
        async function updateMetrics() {
            try {
                const [frontend, backend, database] = await Promise.all([
                    fetch('http://localhost:8080/metrics').then(r => r.json()),
                    fetch('http://localhost:8081/metrics').then(r => r.json()),
                    fetch('http://localhost:8082/metrics').then(r => r.json())
                ]);
                
                // Update summary stats
                document.getElementById('avgLatency').textContent = Math.round(frontend.avg_latency_ms);
                document.getElementById('dnsTime').textContent = Math.round(frontend.avg_dns_ms);
                document.getElementById('threadUtil').textContent = Math.round(frontend.thread_utilization);
                document.getElementById('errorRate').textContent = frontend.error_rate.toFixed(1);
                
                // Update stat card colors
                updateCardColor('latencyCard', frontend.avg_latency_ms, 100, 300);
                updateCardColor('dnsCard', frontend.avg_dns_ms, 50, 200);
                updateCardColor('threadCard', frontend.thread_utilization, 70, 90);
                updateCardColor('errorCard', frontend.error_rate, 5, 15);
                
                // Update service metrics
                document.getElementById('frontendReqs').textContent = frontend.request_count;
                document.getElementById('frontendLatency').textContent = Math.round(frontend.avg_latency_ms);
                document.getElementById('frontendDns').textContent = Math.round(frontend.avg_dns_ms);
                document.getElementById('frontendErrors').textContent = frontend.error_rate.toFixed(1);
                
                document.getElementById('backendReqs').textContent = backend.request_count;
                document.getElementById('backendLatency').textContent = Math.round(backend.avg_latency_ms);
                document.getElementById('backendDns').textContent = Math.round(backend.avg_dns_ms);
                document.getElementById('backendErrors').textContent = backend.error_rate.toFixed(1);
                
                document.getElementById('databaseReqs').textContent = database.request_count;
                document.getElementById('databaseLatency').textContent = Math.round(database.avg_latency_ms);
                
                // Update charts
                const now = new Date().toLocaleTimeString();
                if (latencyChart.data.labels.length > 30) {
                    latencyChart.data.labels.shift();
                    latencyChart.data.datasets[0].data.shift();
                    threadChart.data.labels.shift();
                    threadChart.data.datasets[0].data.shift();
                }
                
                latencyChart.data.labels.push(now);
                latencyChart.data.datasets[0].data.push(frontend.avg_latency_ms);
                latencyChart.update('none');
                
                threadChart.data.labels.push(now);
                threadChart.data.datasets[0].data.push(frontend.thread_utilization);
                threadChart.update('none');
                
            } catch (err) {
                console.error('Failed to fetch metrics:', err);
            }
        }
        
        function updateCardColor(cardId, value, warningThreshold, dangerThreshold) {
            const card = document.getElementById(cardId);
            card.classList.remove('warning', 'danger');
            if (value >= dangerThreshold) {
                card.classList.add('danger');
            } else if (value >= warningThreshold) {
                card.classList.add('warning');
            }
        }
        
        // Start updating
        setInterval(updateMetrics, 1000);
        updateMetrics();
    </script>
</body>
</html>
EOF

cat > dashboard/Dockerfile << 'EOF'
FROM nginx:alpine
COPY index.html /usr/share/nginx/html/
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
EOF

# Create Dockerfiles
echo "🔨 Creating Dockerfiles..."

cat > frontend/Dockerfile << 'EOF'
FROM python:3.11-slim
WORKDIR /app
RUN pip install flask flask-cors requests
COPY app.py .
EXPOSE 8080
CMD ["python", "app.py"]
EOF

cat > backend/Dockerfile << 'EOF'
FROM python:3.11-slim
WORKDIR /app
RUN pip install flask flask-cors requests
COPY app.py .
EXPOSE 8081
CMD ["python", "app.py"]
EOF

cat > database/Dockerfile << 'EOF'
FROM python:3.11-slim
WORKDIR /app
RUN pip install flask flask-cors
COPY app.py .
EXPOSE 8082
CMD ["python", "app.py"]
EOF

cat > dns-server/Dockerfile << 'EOF'
FROM python:3.11-slim
WORKDIR /app
RUN pip install dnslib flask flask-cors
COPY dns_server.py .
EXPOSE 53/udp 5380
CMD ["python", "dns_server.py"]
EOF

cat > loadgen/Dockerfile << 'EOF'
FROM python:3.11-slim
WORKDIR /app
RUN pip install requests
COPY loadgen.py .
CMD ["python", "loadgen.py"]
EOF

# Create docker-compose.yml
echo "🔨 Creating docker-compose.yml..."
cat > docker-compose.yml << 'EOF'
version: '3.8'

services:
  dns:
    build: ./dns-server
    container_name: dns-server
    networks:
      app-network:
        ipv4_address: 172.20.0.2
    ports:
      - "5380:5380"
    cap_add:
      - NET_ADMIN
  
  database:
    build: ./database
    container_name: database
    networks:
      app-network:
        ipv4_address: 172.20.0.12
    ports:
      - "8082:8082"
    dns:
      - 172.20.0.2
    depends_on:
      - dns
  
  backend:
    build: ./backend
    container_name: backend
    networks:
      app-network:
        ipv4_address: 172.20.0.11
    ports:
      - "8081:8081"
    dns:
      - 172.20.0.2
    depends_on:
      - dns
      - database
  
  frontend:
    build: ./frontend
    container_name: frontend
    networks:
      app-network:
        ipv4_address: 172.20.0.10
    ports:
      - "8080:8080"
    dns:
      - 172.20.0.2
    depends_on:
      - dns
      - backend
  
  loadgen:
    build: ./loadgen
    container_name: loadgen
    networks:
      - app-network
    dns:
      - 172.20.0.2
    depends_on:
      - frontend
  
  dashboard:
    build: ./dashboard
    container_name: dashboard
    networks:
      - app-network
    ports:
      - "3000:80"

networks:
  app-network:
    driver: bridge
    ipam:
      config:
        - subnet: 172.20.0.0/16
EOF

# Build and start
echo ""
echo "🏗️  Building Docker images..."
docker-compose build

echo ""
echo "🚀 Starting services..."
docker-compose up -d

echo ""
echo "⏳ Waiting for services to be ready..."
sleep 10

echo ""
echo "✅ DNS Resolution Delay Demo is running!"
echo ""
echo "📊 Dashboard: http://localhost:3000"
echo "🔧 Frontend API: http://localhost:8080"
echo "🔧 Backend API: http://localhost:8081"
echo "🔧 Database API: http://localhost:8082"
echo "🎛️  DNS Control: http://localhost:5380"
echo ""
echo "💡 How to use:"
echo "   1. Open http://localhost:3000 in your browser"
echo "   2. Watch the metrics under normal operation (2-5ms DNS)"
echo "   3. Use the slider to inject DNS delay (try 300ms)"
echo "   4. Observe how latency, thread utilization, and errors spike"
echo "   5. Set delay back to 0 and watch recovery"
echo ""
echo "📝 View logs: docker-compose logs -f"
echo "🛑 Stop demo: docker-compose down"
echo ""

# Go back to parent directory to create helper scripts
cd ..

# Create demo.sh script
cat > demo.sh << 'EOF'
#!/bin/bash

set -e

echo "🚀 Starting DNS Resolution Delay Demo..."
echo "========================================"
echo ""

cd dns-delay-demo || {
    echo "❌ dns-delay-demo directory not found. Please run ./setup.sh first."
    exit 1
}

echo "🏗️  Building Docker images (if needed)..."
docker-compose build

echo ""
echo "🚀 Starting services..."
docker-compose up -d

echo ""
echo "⏳ Waiting for services to be ready..."
sleep 10

echo ""
echo "✅ DNS Resolution Delay Demo is running!"
echo ""
echo "📊 Dashboard: http://localhost:3000"
echo "🔧 Frontend API: http://localhost:8080"
echo "🔧 Backend API: http://localhost:8081"
echo "🔧 Database API: http://localhost:8082"
echo "🎛️  DNS Control: http://localhost:5380"
echo ""
echo "💡 How to use:"
echo "   1. Open http://localhost:3000 in your browser"
echo "   2. Watch the metrics under normal operation (2-5ms DNS)"
echo "   3. Use the slider to inject DNS delay (try 300ms)"
echo "   4. Observe how latency, thread utilization, and errors spike"
echo "   5. Set delay back to 0 and watch recovery"
echo ""
echo "📝 View logs: docker-compose logs -f"
echo "🛑 Stop demo: docker-compose down"
echo ""
EOF

chmod +x demo.sh

echo "✅ Created demo.sh"

# Create cleanup script
cat > cleanup.sh << 'EOF'
#!/bin/bash

echo "🧹 Cleaning up DNS Resolution Delay Demo..."

cd dns-delay-demo 2>/dev/null || {
    echo "❌ Demo directory not found. Nothing to clean up."
    exit 0
}

echo "🛑 Stopping containers..."
docker-compose down -v

echo "🗑️  Removing images..."
docker-compose down --rmi all

cd ..

echo "🗑️  Removing files..."
rm -rf dns-delay-demo

echo "✅ Cleanup complete!"
EOF

chmod +x cleanup.sh

echo "✅ Created cleanup.sh"
echo ""
echo "✨ Setup complete! Run ./demo.sh to start the demo."