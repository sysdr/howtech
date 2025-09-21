from flask import Flask, jsonify
from flask_cors import CORS
import time
import random

app = Flask(__name__)
CORS(app)

@app.route('/fast')
def fast_endpoint():
    return jsonify({"message": "Fast response!"})

@app.route('/slow')
def slow_endpoint():
    time.sleep(0.5)  # Simulate a 500ms delay
    return jsonify({"message": "Slow response!"})

if __name__ == '__main__':
    app.run(port=5000, debug=True)
