#!/bin/bash

echo "🎯 Interactive Sticky Session Tester"
echo "==================================="
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

echo -e "${BLUE}Available Commands:${NC}"
echo "1. Add item to cart"
echo "2. View cart"
echo "3. Test sticky behavior"
echo "4. Health check"
echo "5. Exit"
echo ""

while true; do
    echo -e "${CYAN}Choose a command (1-5):${NC}"
    read -r choice
    
    case $choice in
        1)
            echo -e "${YELLOW}Add item to cart${NC}"
            echo "Enter server port (3001, 3002, or 3003):"
            read -r port
            echo "Enter session ID:"
            read -r session_id
            echo "Enter product ID:"
            read -r product_id
            echo "Enter product name:"
            read -r product_name
            echo "Enter price:"
            read -r price
            echo "Enter quantity (default 1):"
            read -r quantity
            quantity=${quantity:-1}
            
            response=$(make_request "POST" "http://localhost:$port/api/cart/add" "{\"productId\":\"$product_id\",\"name\":\"$product_name\",\"price\":$price,\"quantity\":$quantity}" "$session_id")
            echo -e "${GREEN}Response:${NC}"
            echo "$response" | jq '.' 2>/dev/null || echo "$response"
            echo ""
            ;;
        2)
            echo -e "${YELLOW}View cart${NC}"
            echo "Enter server port (3001, 3002, or 3003):"
            read -r port
            echo "Enter session ID:"
            read -r session_id
            
            response=$(make_request "GET" "http://localhost:$port/api/cart" "" "$session_id")
            echo -e "${GREEN}Response:${NC}"
            echo "$response" | jq '.' 2>/dev/null || echo "$response"
            echo ""
            ;;
        3)
            echo -e "${YELLOW}Test sticky behavior${NC}"
            echo "This will add items to different servers with the same session ID"
            echo "Enter session ID:"
            read -r session_id
            
            echo "Adding iPhone to server 1..."
            response1=$(make_request "POST" "http://localhost:3001/api/cart/add" '{"productId":"iphone","name":"iPhone 15","price":999,"quantity":1}' "$session_id")
            echo "Server 1 response:"
            echo "$response1" | jq '.server' 2>/dev/null || echo "$response1"
            
            echo "Adding AirPods to server 2..."
            response2=$(make_request "POST" "http://localhost:3002/api/cart/add" '{"productId":"airpods","name":"AirPods Pro","price":249,"quantity":1}' "$session_id")
            echo "Server 2 response:"
            echo "$response2" | jq '.server' 2>/dev/null || echo "$response2"
            
            echo "Checking cart on server 1..."
            cart1=$(make_request "GET" "http://localhost:3001/api/cart" "" "$session_id")
            echo "Server 1 cart:"
            echo "$cart1" | jq '.cart' 2>/dev/null || echo "$cart1"
            
            echo "Checking cart on server 2..."
            cart2=$(make_request "GET" "http://localhost:3002/api/cart" "" "$session_id")
            echo "Server 2 cart:"
            echo "$cart2" | jq '.cart' 2>/dev/null || echo "$cart2"
            
            echo -e "${RED}Notice: Same session ID, different servers = different carts!${NC}"
            echo ""
            ;;
        4)
            echo -e "${YELLOW}Health check${NC}"
            for port in 3001 3002 3003; do
                echo -e "${CYAN}Server $port:${NC}"
                response=$(make_request "GET" "http://localhost:$port/health" "")
                echo "$response" | jq '.server' 2>/dev/null || echo "$response"
            done
            echo ""
            ;;
        5)
            echo -e "${GREEN}Goodbye!${NC}"
            exit 0
            ;;
        *)
            echo -e "${RED}Invalid choice. Please enter 1-5.${NC}"
            ;;
    esac
done
