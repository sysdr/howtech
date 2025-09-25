#!/bin/bash
echo "Starting stateless session demo..."
echo "🐳 Starting Redis..."
docker-compose up -d redis

echo "📦 Installing dependencies..."
npm install

echo "🚀 Starting stateless servers..."
SERVER_ID=stateless-server-1 PORT=4001 node stateless-servers/server.js &
SERVER_ID=stateless-server-2 PORT=4002 node stateless-servers/server.js &

echo "Waiting for services to start..."
sleep 5

echo "✅ Demo ready! Test manually or run: node shared/test-client.js"
echo "Servers running on ports 4001, 4002"
echo "Redis running on port 6379"
