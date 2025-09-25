const express = require('express');
const session = require('express-session');
const redis = require('redis');
const RedisStore = require('connect-redis').default;
const cors = require('cors');
const { v4: uuidv4 } = require('uuid');

const app = express();
const PORT = process.env.PORT || 3000;
const SERVER_ID = process.env.SERVER_ID || `server-${PORT}`;
const REDIS_URL = process.env.REDIS_URL || 'redis://localhost:6379';

app.use(cors());
app.use(express.json());

// Redis client setup
const redisClient = redis.createClient({ url: REDIS_URL });

redisClient.on('error', (err) => {
    console.error('Redis connection error:', err);
});

redisClient.on('connect', () => {
    console.log(`[${SERVER_ID}] Connected to Redis`);
});

// Initialize Redis connection
redisClient.connect().catch(console.error);

// Session middleware with Redis store
app.use(session({
    genid: () => uuidv4(),
    secret: 'stateless-demo-secret',
    store: new RedisStore({ client: redisClient }), // The solution!
    resave: false,
    saveUninitialized: false,
    cookie: { secure: false, maxAge: 1800000 } // 30 minutes
}));

// Shopping cart endpoints (identical logic, but stateless!)
app.post('/api/cart/add', (req, res) => {
    const { productId, name, price, quantity = 1 } = req.body;
    
    if (!req.session.cart) {
        req.session.cart = [];
    }
    
    const existingItem = req.session.cart.find(item => item.productId === productId);
    if (existingItem) {
        existingItem.quantity += quantity;
    } else {
        req.session.cart.push({ productId, name, price, quantity });
    }
    
    console.log(`[${SERVER_ID}] Cart updated for session ${req.sessionID}`);
    
    res.json({
        success: true,
        server: SERVER_ID,
        sessionId: req.sessionID,
        cart: req.session.cart,
        cartTotal: req.session.cart.reduce((sum, item) => sum + (item.price * item.quantity), 0)
    });
});

app.get('/api/cart', (req, res) => {
    const cart = req.session.cart || [];
    const cartTotal = cart.reduce((sum, item) => sum + (item.price * item.quantity), 0);
    
    console.log(`[${SERVER_ID}] Cart retrieved for session ${req.sessionID} (any server can handle this!)`);
    
    res.json({
        server: SERVER_ID,
        sessionId: req.sessionID,
        cart,
        cartTotal
    });
});

app.get('/health', (req, res) => {
    res.json({ 
        status: 'healthy', 
        server: SERVER_ID,
        timestamp: new Date().toISOString(),
        sessionStore: 'Redis (distributed)',
        memoryUsage: process.memoryUsage()
    });
});

// Simulate server crash (but sessions survive!)
app.post('/crash', (req, res) => {
    console.log(`[${SERVER_ID}] Simulating crash... (sessions will survive!)`);
    res.json({ message: `${SERVER_ID} going down! Sessions safe in Redis.` });
    setTimeout(() => process.exit(1), 100);
});

app.listen(PORT, () => {
    console.log(`🏪 Stateless server ${SERVER_ID} running on port ${PORT}`);
    console.log(`✅ Using Redis for session storage - sessions survive crashes!`);
});
