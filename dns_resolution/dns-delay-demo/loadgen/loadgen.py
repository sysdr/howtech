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
