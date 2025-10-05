#!/bin/bash

set -e

echo "🚀 Cache Stampede Demo - Automated Setup"
echo "=========================================="
echo ""
echo "This script will:"
echo "  1. Check prerequisites (Node.js, npm, Docker, curl)"
echo "  2. Create a demo project with cache stampede examples"
echo "  3. Start Redis in a Docker container"
echo "  4. Run performance tests comparing different strategies"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check prerequisites
echo "📋 Step 1: Checking prerequisites..."
command -v node >/dev/null 2>&1 || { echo -e "${RED}Node.js is required but not installed. Install from nodejs.org${NC}" >&2; exit 1; }
command -v npm >/dev/null 2>&1 || { echo -e "${RED}npm is required but not installed.${NC}" >&2; exit 1; }
command -v docker >/dev/null 2>&1 || { echo -e "${RED}Docker is required but not installed. Install from docker.com${NC}" >&2; exit 1; }
command -v curl >/dev/null 2>&1 || { echo -e "${RED}curl is required but not installed.${NC}" >&2; exit 1; }

# Check if Docker daemon is running
echo "🐳 Checking Docker daemon..."
if ! docker info >/dev/null 2>&1; then
  echo -e "${RED}Docker daemon is not running. Please start Docker and try again.${NC}" >&2
  echo -e "${YELLOW}On macOS: Start Docker Desktop application${NC}" >&2
  echo -e "${YELLOW}On Linux: Run 'sudo systemctl start docker'${NC}" >&2
  exit 1
fi

echo -e "${GREEN}✓ All prerequisites found${NC}"
echo ""

# Create project structure
echo "📁 Step 2: Creating project structure..."
PROJECT_DIR="cache-stampede-demo"
rm -rf $PROJECT_DIR
mkdir -p $PROJECT_DIR
cd $PROJECT_DIR

echo -e "${GREEN}✓ Project directory created${NC}"
echo ""

# Generate package.json
echo "📦 Step 3: Generating package.json..."
cat > package.json << 'EOF'
{
  "name": "cache-stampede-demo",
  "version": "1.0.0",
  "description": "Hands-on demo of cache stampede and mitigation strategies",
  "main": "server.js",
  "scripts": {
    "start": "node server.js",
    "test": "node test-stampede.js"
  },
  "dependencies": {
    "express": "^4.18.2",
    "ioredis": "^5.3.2",
    "autocannon": "^7.12.0"
  }
}
EOF

echo -e "${GREEN}✓ package.json created${NC}"
echo ""

# Generate server.js - Main application
echo "🔧 Step 4: Generating server application..."
cat > server.js << 'EOF'
const express = require('express');
const Redis = require('ioredis');

const app = express();
const redis = new Redis({ host: 'localhost', port: 6379 });

// Simulated database - intentionally slow
let dbQueryCount = 0;
async function queryDatabase(key) {
  dbQueryCount++;
  console.log(`💾 DB Query #${dbQueryCount} for key: ${key}`);
  await new Promise(resolve => setTimeout(resolve, 500)); // 500ms latency
  return { data: `Data for ${key}`, timestamp: Date.now() };
}

// PROBLEM: Naive implementation (vulnerable to stampede)
app.get('/api/naive/:key', async (req, res) => {
  const key = req.params.key;
  const cached = await redis.get(`naive:${key}`);
  
  if (cached) {
    return res.json({ source: 'cache', data: JSON.parse(cached) });
  }
  
  // Cache miss - every request hits the database!
  const data = await queryDatabase(key);
  await redis.setex(`naive:${key}`, 5, JSON.stringify(data));
  res.json({ source: 'database', data });
});

// SOLUTION 1: Request Coalescing with Distributed Lock
const pendingRequests = new Map();

app.get('/api/coalescing/:key', async (req, res) => {
  const key = req.params.key;
  const cacheKey = `coalescing:${key}`;
  const lockKey = `lock:${key}`;
  
  // Check cache first
  const cached = await redis.get(cacheKey);
  if (cached) {
    return res.json({ source: 'cache', data: JSON.parse(cached) });
  }
  
  // Try to acquire lock (only one request regenerates)
  const lockAcquired = await redis.set(lockKey, '1', 'EX', 10, 'NX');
  
  if (lockAcquired) {
    // This request won the race - regenerate data
    try {
      const data = await queryDatabase(key);
      await redis.setex(cacheKey, 5, JSON.stringify(data));
      await redis.del(lockKey);
      res.json({ source: 'database', data, role: 'generator' });
    } catch (err) {
      await redis.del(lockKey);
      throw err;
    }
  } else {
    // Wait for the generator to finish
    let attempts = 0;
    while (attempts < 20) {
      await new Promise(resolve => setTimeout(resolve, 100));
      const cached = await redis.get(cacheKey);
      if (cached) {
        return res.json({ source: 'cache', data: JSON.parse(cached), role: 'waiter' });
      }
      attempts++;
    }
    // Fallback if something went wrong
    const data = await queryDatabase(key);
    res.json({ source: 'database', data, role: 'fallback' });
  }
});

// SOLUTION 2: Probabilistic Early Expiration
app.get('/api/probabilistic/:key', async (req, res) => {
  const key = req.params.key;
  const cacheKey = `prob:${key}`;
  const metaKey = `prob:meta:${key}`;
  
  const cached = await redis.get(cacheKey);
  const meta = await redis.get(metaKey);
  
  if (cached && meta) {
    const { created, ttl } = JSON.parse(meta);
    const age = (Date.now() - created) / 1000;
    const timeLeft = ttl - age;
    
    // Probabilistic refresh: higher chance as expiration approaches
    const beta = 1.5;
    const refreshProbability = (age / ttl) * beta;
    
    if (Math.random() < refreshProbability) {
      // Refresh in background, return cached data immediately
      queryDatabase(key).then(data => {
        const metadata = { created: Date.now(), ttl: 5 };
        redis.setex(cacheKey, 5, JSON.stringify(data));
        redis.setex(metaKey, 5, JSON.stringify(metadata));
      });
    }
    
    return res.json({ source: 'cache', data: JSON.parse(cached), timeLeft });
  }
  
  // Cache miss - regenerate
  const data = await queryDatabase(key);
  const metadata = { created: Date.now(), ttl: 5 };
  await redis.setex(cacheKey, 5, JSON.stringify(data));
  await redis.setex(metaKey, 5, JSON.stringify(metadata));
  res.json({ source: 'database', data });
});

// Reset endpoint for testing
app.post('/reset', async (req, res) => {
  await redis.flushall();
  dbQueryCount = 0;
  res.json({ message: 'Reset complete' });
});

// Stats endpoint
app.get('/stats', (req, res) => {
  res.json({ dbQueryCount });
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log(`🌐 Server running on http://localhost:${PORT}`);
  console.log(`📊 Stats available at http://localhost:${PORT}/stats`);
});
EOF

echo -e "${GREEN}✓ server.js created${NC}"
echo ""

# Generate test script
echo "🧪 Step 5: Generating test script..."
cat > test-stampede.js << 'EOF'
const autocannon = require('autocannon');
const http = require('http');

// Helper to reset server state
function resetServer() {
  return new Promise((resolve) => {
    const req = http.request({
      hostname: 'localhost',
      port: 3000,
      path: '/reset',
      method: 'POST'
    }, (res) => {
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => resolve(JSON.parse(data)));
    });
    req.end();
  });
}

// Helper to get stats
function getStats() {
  return new Promise((resolve) => {
    http.get('http://localhost:3000/stats', (res) => {
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => resolve(JSON.parse(data)));
    });
  });
}

async function runTest(endpoint, name) {
  console.log(`\n${'='.repeat(60)}`);
  console.log(`🧪 Testing: ${name}`);
  console.log('='.repeat(60));
  
  await resetServer();
  
  // Wait for cache to clear
  await new Promise(resolve => setTimeout(resolve, 1000));
  
  const result = await autocannon({
    url: `http://localhost:3000${endpoint}`,
    connections: 100,
    duration: 3,
    pipelining: 1
  });
  
  const stats = await getStats();
  
  console.log(`\n📊 Results:`);
  console.log(`  Requests: ${result.requests.total}`);
  console.log(`  Database Queries: ${stats.dbQueryCount}`);
  console.log(`  Cache Efficiency: ${((1 - stats.dbQueryCount/result.requests.total) * 100).toFixed(2)}%`);
  console.log(`  Avg Latency: ${result.latency.mean.toFixed(2)}ms`);
  console.log(`  Requests/sec: ${result.requests.average}`);
  
  return stats.dbQueryCount;
}

async function main() {
  console.log('🚀 Cache Stampede Comparison Test\n');
  console.log('This test simulates 100 concurrent users making requests');
  console.log('for 3 seconds, comparing different caching strategies.\n');
  
  // Wait for server to be ready
  await new Promise(resolve => setTimeout(resolve, 2000));
  
  const naiveQueries = await runTest('/api/naive/popular-item', 'NAIVE (Stampede-Prone)');
  await new Promise(resolve => setTimeout(resolve, 2000));
  
  const coalescingQueries = await runTest('/api/coalescing/popular-item', 'REQUEST COALESCING');
  await new Promise(resolve => setTimeout(resolve, 2000));
  
  const probQueries = await runTest('/api/probabilistic/popular-item', 'PROBABILISTIC EXPIRATION');
  
  console.log(`\n${'='.repeat(60)}`);
  console.log('📈 FINAL COMPARISON');
  console.log('='.repeat(60));
  console.log(`Naive Implementation:         ${naiveQueries} DB queries`);
  console.log(`Request Coalescing:          ${coalescingQueries} DB queries (${((1 - coalescingQueries/naiveQueries) * 100).toFixed(1)}% reduction)`);
  console.log(`Probabilistic Expiration:    ${probQueries} DB queries (${((1 - probQueries/naiveQueries) * 100).toFixed(1)}% reduction)`);
  console.log('\n✅ Test complete! Check the results above.\n');
  
  process.exit(0);
}

main().catch(console.error);
EOF

echo -e "${GREEN}✓ test-stampede.js created${NC}"
echo ""

# Start Redis with Docker
echo "🐳 Step 6: Starting Redis with Docker..."
CONTAINER_NAME="redis-stampede"

# Clean up any existing container with the same name
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "🗑️  Removing existing container..."
  docker rm -f ${CONTAINER_NAME} >/dev/null 2>&1 || true
fi

# Start Redis container
echo "🚀 Starting Redis container..."
if docker run -d --name ${CONTAINER_NAME} -p 6379:6379 redis:alpine >/dev/null 2>&1; then
  echo -e "${GREEN}✓ Redis container started successfully${NC}"
else
  echo -e "${RED}Failed to start Redis container${NC}" >&2
  exit 1
fi

# Wait for Redis to be ready
echo "⏳ Waiting for Redis to be ready..."
sleep 3

# Test Redis connection
if docker exec ${CONTAINER_NAME} redis-cli ping >/dev/null 2>&1; then
  echo -e "${GREEN}✓ Redis is ready and responding${NC}"
else
  echo -e "${YELLOW}⚠️  Redis may not be fully ready yet, but continuing...${NC}"
fi
echo ""

# Install dependencies
echo "📥 Step 7: Installing npm dependencies..."
if npm install --silent; then
  echo -e "${GREEN}✓ Dependencies installed${NC}"
else
  echo -e "${RED}Failed to install dependencies${NC}" >&2
  exit 1
fi
echo ""

# Start the server in background
echo "🌐 Step 8: Starting application server..."
node server.js &
SERVER_PID=$!

# Wait for server to start and test if it's responding
echo "⏳ Waiting for server to start..."
sleep 3

# Test if server is responding
if curl -s http://localhost:3000/stats >/dev/null 2>&1; then
  echo -e "${GREEN}✓ Server running and responding (PID: $SERVER_PID)${NC}"
else
  echo -e "${YELLOW}⚠️  Server may not be fully ready yet, but continuing...${NC}"
fi
echo ""

# Run tests
echo "🧪 Step 9: Running stampede comparison tests..."
echo ""
if npm test; then
  echo -e "${GREEN}✓ Tests completed successfully${NC}"
else
  echo -e "${YELLOW}⚠️  Tests completed with warnings or errors${NC}"
fi

# Cleanup
echo ""
echo "🧹 Cleaning up..."
kill $SERVER_PID 2>/dev/null || true
docker stop ${CONTAINER_NAME} >/dev/null 2>&1 || true

echo ""
echo -e "${GREEN}=========================================="
echo "✅ SETUP COMPLETE!"
echo "==========================================${NC}"
echo ""
echo "📁 Project created in: ./$PROJECT_DIR"
echo ""
echo "To run again manually:"
echo "  1. cd $PROJECT_DIR"
echo "  2. docker start ${CONTAINER_NAME}"
echo "  3. node server.js &"
echo "  4. npm test"
echo ""
echo "🌐 Server endpoints:"
echo "  http://localhost:3000/api/naive/:key"
echo "  http://localhost:3000/api/coalescing/:key"
echo "  http://localhost:3000/api/probabilistic/:key"
echo ""