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
