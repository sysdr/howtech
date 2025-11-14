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
