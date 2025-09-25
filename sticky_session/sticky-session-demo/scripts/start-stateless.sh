#!/bin/bash
echo "🚀 Starting Stateless Session Demo..."

# Start Redis
docker-compose up -d redis

# Wait for Redis to be ready
echo "Waiting for Redis to be ready..."
sleep 5

# Start three stateless servers
SERVER_ID=stateless-1 PORT=3001 node src/stateless-server.js &
SERVER_ID=stateless-2 PORT=3002 node src/stateless-server.js &
SERVER_ID=stateless-3 PORT=3003 node src/stateless-server.js &

# Start load balancer
sleep 2
node src/load-balancer.js &

echo "✅ Stateless session environment running!"
echo "🔗 Access via: http://localhost:8080/stateless/api/cart"
echo "📊 Health check: http://localhost:8080/stateless/health"

wait
