#!/bin/bash
echo "🚀 Starting Sticky Session Demo..."

# Start Redis
docker-compose up -d redis

# Start three sticky session servers
SERVER_ID=sticky-1 PORT=3001 node src/sticky-server.js &
SERVER_ID=sticky-2 PORT=3002 node src/sticky-server.js &
SERVER_ID=sticky-3 PORT=3003 node src/sticky-server.js &

# Start load balancer
sleep 2
node src/load-balancer.js &

echo "✅ Sticky session environment running!"
echo "🔗 Access via: http://localhost:8080/sticky/api/cart"
echo "📊 Health check: http://localhost:8080/sticky/health"

wait
