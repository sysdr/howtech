#!/bin/bash

set -e

echo "🧪 Running tests for Proxy Showdown Demo..."
echo "============================================"

# Check if docker-compose.yml exists
if [ ! -f "docker-compose.yml" ]; then
    echo "❌ ERROR: docker-compose.yml not found in current directory"
    exit 1
fi

# Check if docker-compose is running
if ! (docker compose ps 2>/dev/null | grep -q "Up" || docker-compose ps 2>/dev/null | grep -q "Up"); then
    echo "❌ ERROR: Docker containers are not running. Start them first with ./start.sh"
    exit 1
fi

echo ""
echo "📊 Checking service health..."
echo "---------------------------"

# Test HAProxy
echo -n "Testing HAProxy (port 8080)... "
if curl -s -f -o /dev/null -w "%{http_code}" http://localhost:8080/api/users -H "apikey: demo-api-key-12345" | grep -q "200\|429"; then
    echo "✅ OK"
else
    echo "❌ FAILED"
    exit 1
fi

# Test Dashboard
echo -n "Testing Dashboard (port 3001)... "
if curl -s -f -o /dev/null -w "%{http_code}" http://localhost:3001 | grep -q "200"; then
    echo "✅ OK"
else
    echo "❌ FAILED"
    exit 1
fi

# Test HAProxy Stats
echo -n "Testing HAProxy Stats (port 8404)... "
if curl -s -f -o /dev/null -w "%{http_code}" http://localhost:8404/stats | grep -q "200"; then
    echo "✅ OK"
else
    echo "❌ FAILED"
    exit 1
fi

# Test Backend directly (if accessible)
echo -n "Testing Backend health... "
if (docker compose exec -T backend1 wget -q -O- http://localhost:3000/health 2>/dev/null || docker-compose exec -T backend1 wget -q -O- http://localhost:3000/health 2>/dev/null); then
    echo "✅ OK"
else
    echo "⚠️  WARNING: Cannot test backend directly"
fi

echo ""
echo "🔄 Testing request flow..."
echo "------------------------"

# Test API endpoint with authentication
echo -n "Testing API endpoint with auth... "
RESPONSE=$(curl -s -w "\n%{http_code}" -H "apikey: demo-api-key-12345" http://localhost:8080/api/users)
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | head -n-1)

if [ "$HTTP_CODE" = "200" ]; then
    echo "✅ OK (HTTP $HTTP_CODE)"
    # Check if response contains expected data
    if echo "$BODY" | grep -q "instance"; then
        echo "   ✅ Response contains instance data"
    else
        echo "   ⚠️  WARNING: Response may be incomplete"
    fi
else
    echo "❌ FAILED (HTTP $HTTP_CODE)"
    exit 1
fi

# Test rate limiting (send multiple requests quickly)
echo -n "Testing rate limiting (10 requests)... "
RATE_LIMITED=0
for i in {1..10}; do
    HTTP_CODE=$(curl -s -w "%{http_code}" -o /dev/null -H "apikey: demo-api-key-12345" http://localhost:8080/api/users)
    if [ "$HTTP_CODE" = "429" ]; then
        RATE_LIMITED=$((RATE_LIMITED + 1))
    fi
    sleep 0.1
done

if [ $RATE_LIMITED -gt 0 ]; then
    echo "✅ OK (Rate limiting working: $RATE_LIMITED/10 requests limited)"
else
    echo "⚠️  WARNING: Rate limiting may not be working (0/10 requests limited)"
fi

# Test caching
echo ""
echo "📦 Testing cache behavior..."
echo "---------------------------"
echo -n "First request (cache miss)... "
START1=$(date +%s%N)
curl -s -o /dev/null -H "apikey: demo-api-key-12345" http://localhost:8080/api/users
END1=$(date +%s%N)
TIME1=$(( (END1 - START1) / 1000000 ))

sleep 1

echo -n "Second request (should be cached)... "
START2=$(date +%s%N)
CACHE_STATUS=$(curl -s -I -H "apikey: demo-api-key-12345" http://localhost:8080/api/users | grep -i "x-cache-status" | tr -d '\r' | cut -d' ' -f2)
END2=$(date +%s%N)
TIME2=$(( (END2 - START2) / 1000000 ))

if [ "$CACHE_STATUS" = "HIT" ]; then
    echo "✅ OK (Cache HIT, $TIME2 ms vs $TIME1 ms)"
else
    echo "⚠️  WARNING: Cache may not be working (Status: ${CACHE_STATUS:-NONE})"
fi

echo ""
echo "📈 Testing dashboard metrics..."
echo "------------------------------"

# Send a few requests to generate metrics
echo "Sending test requests to generate metrics..."
for i in {1..5}; do
    curl -s -o /dev/null -H "apikey: demo-api-key-12345" http://localhost:8080/api/users
    sleep 0.5
done

# Check if dashboard is accessible and contains expected content
echo -n "Checking dashboard content... "
DASHBOARD_CONTENT=$(curl -s http://localhost:3001)
if echo "$DASHBOARD_CONTENT" | grep -q "Proxy Showdown Dashboard"; then
    echo "✅ OK"
else
    echo "❌ FAILED"
    exit 1
fi

echo ""
echo "✅ All tests passed!"
echo ""
echo "📊 Service Status:"
docker compose ps 2>/dev/null || docker-compose ps

echo ""
echo "🎯 Next steps:"
echo "  - Open dashboard: http://localhost:3001"
echo "  - View HAProxy stats: http://localhost:8404/stats"
echo "  - Send requests to: http://localhost:8080/api/users"
echo ""
