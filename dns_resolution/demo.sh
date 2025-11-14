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
