#!/bin/sh
set -e

# Wait for Kong to be ready
until curl -s http://localhost:8001/ > /dev/null; do
  sleep 1
done

# Create service
curl -s -X POST http://localhost:8001/services \
  -d "name=backend-service" \
  -d "url=http://nginx:80" > /dev/null

# Create route
SERVICE_ID=$(curl -s http://localhost:8001/services/backend-service | grep -o '"id":"[^"]*"' | head -1 | cut -d'"' -f4)
curl -s -X POST "http://localhost:8001/services/$SERVICE_ID/routes" \
  -d "name=backend-route" \
  -d "paths[]=/api" \
  -d "strip_path=false" > /dev/null

# Create consumer
curl -s -X POST http://localhost:8001/consumers \
  -d "username=demo-user" > /dev/null

# Create API key for consumer
curl -s -X POST "http://localhost:8001/consumers/demo-user/key-auth" \
  -d "key=demo-api-key-12345" > /dev/null

# Enable key-auth plugin on service
curl -s -X POST "http://localhost:8001/services/$SERVICE_ID/plugins" \
  -d "name=key-auth" \
  -d "config.hide_credentials=true" \
  -d "config.key_names[]=apikey" > /dev/null

# Enable rate-limiting plugin
curl -s -X POST "http://localhost:8001/services/$SERVICE_ID/plugins" \
  -d "name=rate-limiting" \
  -d "config.minute=10" \
  -d "config.policy=local" > /dev/null

# Enable correlation-id plugin
curl -s -X POST "http://localhost:8001/services/$SERVICE_ID/plugins" \
  -d "name=correlation-id" \
  -d "config.header_name=X-Request-ID" \
  -d "config.echo_downstream=true" > /dev/null

# Enable CORS plugin
curl -s -X POST "http://localhost:8001/services/$SERVICE_ID/plugins" \
  -d "name=cors" \
  -d "config.origins[]=http://localhost:3001" \
  -d "config.methods[]=GET" \
  -d "config.methods[]=POST" \
  -d "config.methods[]=PUT" \
  -d "config.methods[]=DELETE" \
  -d "config.methods[]=OPTIONS" \
  -d "config.headers[]=Accept" \
  -d "config.headers[]=Content-Type" \
  -d "config.headers[]=apikey" \
  -d "config.headers[]=Authorization" \
  -d "config.exposed_headers[]=X-Request-ID" \
  -d "config.credentials=true" \
  -d "config.max_age=3600" > /dev/null

# Enable prometheus plugin globally
curl -s -X POST http://localhost:8001/plugins \
  -d "name=prometheus" \
  -d "config.status_code_metrics=true" \
  -d "config.latency_metrics=true" > /dev/null

echo "Kong configuration completed"

