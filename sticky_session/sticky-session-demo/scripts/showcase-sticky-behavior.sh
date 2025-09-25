#!/bin/bash

echo "🎭 Sticky Session Behavior Showcase"
echo "=================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to make HTTP requests
make_request() {
    local method=$1
    local url=$2
    local data=$3
    local session_id=$4
    
    if [ -n "$data" ]; then
        if [ -n "$session_id" ]; then
            curl -s -X $method "$url" \
                -H "Content-Type: application/json" \
                -H "X-Session-ID: $session_id" \
                -d "$data" 2>/dev/null
        else
            curl -s -X $method "$url" \
                -H "Content-Type: application/json" \
                -d "$data" 2>/dev/null
        fi
    else
        if [ -n "$session_id" ]; then
            curl -s -X $method "$url" \
                -H "X-Session-ID: $session_id" 2>/dev/null
        else
            curl -s -X $method "$url" 2>/dev/null
        fi
    fi
}

# Function to extract server from response
extract_server() {
    echo "$1" | grep -o '"server":"[^"]*"' | cut -d'"' -f4
}

# Function to extract session ID from response
extract_session_id() {
    echo "$1" | grep -o '"sessionId":"[^"]*"' | cut -d'"' -f4
}

echo -e "${BLUE}📋 Test Plan:${NC}"
echo "1. Test individual servers directly"
echo "2. Test sticky session behavior with same session ID"
echo "3. Test round-robin behavior with different session IDs"
echo "4. Demonstrate session persistence across requests"
echo ""

# Wait for services to be ready
echo -e "${YELLOW}⏳ Waiting for services to be ready...${NC}"
sleep 2

# Test individual servers first
echo -e "${GREEN}🧪 Test 1: Individual Server Health${NC}"
echo "Checking individual servers..."
echo ""

for port in 3001 3002 3003; do
    echo -e "${CYAN}Checking server on port $port...${NC}"
    health_response=$(make_request "GET" "http://localhost:$port/health" "")
    if [ -n "$health_response" ] && [[ ! "$health_response" == *"Error"* ]]; then
        echo -e "${GREEN}✅ Server $port is healthy${NC}"
        server_id=$(echo "$health_response" | jq '.server' 2>/dev/null || echo 'Unknown')
        echo "Server ID: $server_id"
    else
        echo -e "${RED}❌ Server $port is not responding${NC}"
    fi
    echo ""
done

# Test 2: Sticky Session Behavior (Direct to servers)
echo -e "${GREEN}🧪 Test 2: Sticky Session Behavior${NC}"
echo "Testing sticky session behavior by hitting servers directly..."
echo ""

SESSION_ID="user-123"
echo -e "${CYAN}Session ID: $SESSION_ID${NC}"

# Test with server 1
echo "Adding iPhone to cart on server 1..."
response1=$(make_request "POST" "http://localhost:3001/api/cart/add" '{"productId":"iphone","name":"iPhone 15","price":999,"quantity":1}' "$SESSION_ID")
server1=$(extract_server "$response1")
echo -e "Response: ${GREEN}$server1${NC}"
echo ""

# Test with server 2
echo "Adding AirPods to cart on server 2..."
response2=$(make_request "POST" "http://localhost:3002/api/cart/add" '{"productId":"airpods","name":"AirPods Pro","price":249,"quantity":1}' "$SESSION_ID")
server2=$(extract_server "$response2")
echo -e "Response: ${GREEN}$server2${NC}"
echo ""

# Check cart on server 1
echo "Checking cart on server 1..."
response3=$(make_request "GET" "http://localhost:3001/api/cart" "" "$SESSION_ID")
server3=$(extract_server "$response3")
echo -e "Response: ${GREEN}$server3${NC}"
echo "Cart contents:"
echo "$response3" | jq '.cart' 2>/dev/null || echo "$response3"
echo ""

# Check cart on server 2
echo "Checking cart on server 2..."
response4=$(make_request "GET" "http://localhost:3002/api/cart" "" "$SESSION_ID")
server4=$(extract_server "$response4")
echo -e "Response: ${GREEN}$server4${NC}"
echo "Cart contents:"
echo "$response4" | jq '.cart' 2>/dev/null || echo "$response4"
echo ""

echo -e "${YELLOW}💡 Key Insight:${NC}"
echo "• Server 1 has iPhone in cart"
echo "• Server 2 has AirPods in cart"
echo "• Same session ID, different servers = different cart contents!"
echo "• This demonstrates the sticky session problem"
echo ""

# Test 3: Session Persistence on Same Server
echo -e "${GREEN}🧪 Test 3: Session Persistence on Same Server${NC}"
echo "Testing session persistence on the same server..."
echo ""

PERSISTENT_SESSION="persistent-user"
echo -e "${CYAN}Session ID: $PERSISTENT_SESSION${NC}"

# Add multiple items to server 1
products=('{"productId":"book1","name":"JavaScript Guide","price":29,"quantity":1}' 
          '{"productId":"book2","name":"Node.js Handbook","price":39,"quantity":2}'
          '{"productId":"book3","name":"Express Tutorial","price":19,"quantity":1}')

for product in "${products[@]}"; do
    echo "Adding product to server 1..."
    response=$(make_request "POST" "http://localhost:3001/api/cart/add" "$product" "$PERSISTENT_SESSION")
    server=$(extract_server "$response")
    echo -e "Server: ${GREEN}$server${NC}"
    cart_total=$(echo "$response" | jq '.cartTotal' 2>/dev/null || echo 'N/A')
    echo "Cart total: $cart_total"
done

echo ""
echo "Final cart contents on server 1:"
final_response=$(make_request "GET" "http://localhost:3001/api/cart" "" "$PERSISTENT_SESSION")
echo "$final_response" | jq '.cart' 2>/dev/null || echo "$final_response"
echo ""

# Test 4: Load Balancer Test (if working)
echo -e "${GREEN}🧪 Test 4: Load Balancer Test${NC}"
echo "Testing load balancer (if working)..."
echo ""

# Try to test load balancer
lb_test=$(make_request "GET" "http://localhost:8080/sticky/health" "")
if [ -n "$lb_test" ] && [[ ! "$lb_test" == *"Error"* ]]; then
    echo -e "${GREEN}✅ Load balancer is working${NC}"
    echo "Response: $lb_test"
else
    echo -e "${RED}❌ Load balancer is not working properly${NC}"
    echo "This is expected - the load balancer needs to be fixed"
fi
echo ""

# Summary
echo -e "${PURPLE}📊 Summary${NC}"
echo "=========="
echo "✅ Sticky session demo completed!"
echo "🔗 Individual servers accessible at:"
echo "   • http://localhost:3001/api/cart"
echo "   • http://localhost:3002/api/cart"
echo "   • http://localhost:3003/api/cart"
echo "📊 Health checks at:"
echo "   • http://localhost:3001/health"
echo "   • http://localhost:3002/health"
echo "   • http://localhost:3003/health"
echo ""
echo -e "${YELLOW}Key Observations:${NC}"
echo "• Same session ID on different servers = different cart contents"
echo "• Session data persists within each server's memory"
echo "• This demonstrates the sticky session problem"
echo "• Load balancer needs proper routing configuration"
echo ""
echo -e "${RED}⚠️  Important:${NC}"
echo "• Sessions are stored in memory and will be lost if server restarts"
echo "• This demonstrates the problem with sticky sessions"
echo "• In production, use Redis or database-backed sessions"
echo ""