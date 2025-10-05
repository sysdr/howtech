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
