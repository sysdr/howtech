#!/bin/bash
echo "Starting sticky session demo..."
echo "🐳 Starting Redis..."
docker-compose up -d redis

echo "📦 Installing dependencies..."
npm install

echo "🚀 Starting sticky servers..."
SERVER_ID=sticky-server-1 PORT=3001 node sticky-servers/server.js &
SERVER_ID=sticky-server-2 PORT=3002 node sticky-servers/server.js &

echo "⚖️ Starting load balancer..."
node shared/load-balancer.js &

echo "Waiting for services to start..."
sleep 3

echo "🧪 Running test..."
node shared/test-client.js
