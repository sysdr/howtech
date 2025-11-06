#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🚀 Starting Proxy Showdown Demo..."
echo "==================================="

# Check if docker-compose.yml exists
if [ ! -f "docker-compose.yml" ]; then
    echo "❌ ERROR: docker-compose.yml not found in current directory"
    echo "   Current directory: $SCRIPT_DIR"
    exit 1
fi

# Check for duplicate services (containers running on same ports)
echo ""
echo "🔍 Checking for duplicate services..."
echo "------------------------------------"

# Check port 8080 (HAProxy)
if lsof -i :8080 > /dev/null 2>&1 || netstat -tuln 2>/dev/null | grep -q ":8080"; then
    echo "⚠️  WARNING: Port 8080 is already in use"
    echo "   Checking if it's our HAProxy service..."
    if docker ps --format "table {{.Names}}\t{{.Ports}}" | grep -q "8080.*haproxy\|proxy-demo.*haproxy"; then
        echo "   ✅ Port 8080 is used by our HAProxy container"
    else
        echo "   ❌ Port 8080 is used by another service. Please stop it first."
        exit 1
    fi
fi

# Check port 3001 (Dashboard)
if lsof -i :3001 > /dev/null 2>&1 || netstat -tuln 2>/dev/null | grep -q ":3001"; then
    echo "⚠️  WARNING: Port 3001 is already in use"
    echo "   Checking if it's our Dashboard service..."
    if docker ps --format "table {{.Names}}\t{{.Ports}}" | grep -q "3001.*dashboard\|proxy-demo.*dashboard"; then
        echo "   ✅ Port 3001 is used by our Dashboard container"
    else
        echo "   ❌ Port 3001 is used by another service. Please stop it first."
        exit 1
    fi
fi

# Check port 8404 (HAProxy Stats)
if lsof -i :8404 > /dev/null 2>&1 || netstat -tuln 2>/dev/null | grep -q ":8404"; then
    echo "⚠️  WARNING: Port 8404 is already in use"
    echo "   Checking if it's our HAProxy Stats service..."
    if docker ps --format "table {{.Names}}\t{{.Ports}}" | grep -q "8404.*haproxy\|proxy-demo.*haproxy"; then
        echo "   ✅ Port 8404 is used by our HAProxy container"
    else
        echo "   ❌ Port 8404 is used by another service. Please stop it first."
        exit 1
    fi
fi

# Check for existing docker-compose services
echo ""
echo "🔍 Checking for existing Docker Compose services..."
if docker compose ps 2>/dev/null | grep -q "Up" || docker-compose ps 2>/dev/null | grep -q "Up"; then
    echo "⚠️  Found existing running containers. Stopping them first..."
    docker compose down 2>/dev/null || docker-compose down
    sleep 2
fi

# Start services
echo ""
echo "🚀 Starting services..."
docker compose up -d 2>/dev/null || docker-compose up -d

echo ""
echo "⏳ Waiting for services to be healthy..."
sleep 20

# Wait for services to be ready
echo ""
echo "🔍 Checking service health..."
MAX_RETRIES=30
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    # Check HAProxy
    if curl -s -f -o /dev/null http://localhost:8080/api/users -H "apikey: demo-api-key-12345" 2>/dev/null; then
        echo "✅ HAProxy is ready"
        break
    fi
    
    RETRY_COUNT=$((RETRY_COUNT + 1))
    if [ $RETRY_COUNT -lt $MAX_RETRIES ]; then
        echo "   Waiting for services... ($RETRY_COUNT/$MAX_RETRIES)"
        sleep 2
    else
        echo "❌ ERROR: Services did not become healthy in time"
        echo "   Checking container status..."
        docker compose ps 2>/dev/null || docker-compose ps
        echo ""
        echo "   Checking logs..."
        docker compose logs --tail=50 2>/dev/null || docker-compose logs --tail=50
        exit 1
    fi
done

# Check Dashboard
if curl -s -f -o /dev/null http://localhost:3001 2>/dev/null; then
    echo "✅ Dashboard is ready"
else
    echo "⚠️  WARNING: Dashboard may not be ready yet"
fi

echo ""
echo "✅ All services started successfully!"
echo ""
echo "📊 Service Status:"
docker compose ps 2>/dev/null || docker-compose ps

echo ""
echo "======================================"
echo "🌐 Dashboard: http://localhost:3001"
echo "📊 HAProxy Stats: http://localhost:8404/stats"
echo "🔧 API Endpoint: http://localhost:8080/api/users"
echo "======================================"
echo ""
echo "💡 To view logs: docker compose logs -f"
echo "💡 To stop services: docker compose down"
echo ""
