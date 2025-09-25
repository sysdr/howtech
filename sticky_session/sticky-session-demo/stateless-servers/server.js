const express = require('express');
const session = require('express-session');
const RedisStore = require('connect-redis').default;
const { createClient } = require('redis');

const app = express();
const PORT = process.env.PORT || 4001;
const SERVER_ID = process.env.SERVER_ID || 'stateless-server-1';

// Create Redis client
const redisClient = createClient({
  host: 'localhost',
  port: 6379
});

redisClient.on('error', (err) => {
  console.error('Redis Client Error', err);
});

redisClient.connect();

// Redis-backed session store (the solution!)
app.use(session({
  store: new RedisStore({ client: redisClient }),
  secret: 'stateless-session-secret',
  resave: false,
  saveUninitialized: false,
  cookie: { maxAge: 300000 } // 5 minutes
}));

app.use(express.json());

// Simulate shopping cart (same endpoints as sticky version)
app.post('/cart/add', (req, res) => {
  const { item, price } = req.body;
  
  if (!req.session.cart) {
    req.session.cart = [];
    req.session.total = 0;
  }
  
  req.session.cart.push({ item, price });
  req.session.total += price;
  
  console.log(`[${SERVER_ID}] Added ${item} to cart. Session: ${req.sessionID}`);
  
  res.json({
    serverId: SERVER_ID,
    sessionId: req.sessionID,
    cart: req.session.cart,
    total: req.session.total
  });
});

app.get('/cart', (req, res) => {
  console.log(`[${SERVER_ID}] Getting cart. Session: ${req.sessionID}`);
  
  res.json({
    serverId: SERVER_ID,
    sessionId: req.sessionID,
    cart: req.session.cart || [],
    total: req.session.total || 0
  });
});

// Health check
app.get('/health', (req, res) => {
  res.json({ serverId: SERVER_ID, status: 'healthy' });
});

// Simulate server crash (but sessions survive!)
app.post('/crash', (req, res) => {
  console.log(`[${SERVER_ID}] CRASH INITIATED! Sessions are safe in Redis...`);
  res.json({ message: `${SERVER_ID} is going down, but your cart is safe!` });
  setTimeout(() => {
    console.log(`[${SERVER_ID}] 💥 CRASHED - But sessions live on in Redis!`);
    process.exit(1);
  }, 1000);
});

app.listen(PORT, () => {
  console.log(`[${SERVER_ID}] Stateless server running on port ${PORT}`);
});
