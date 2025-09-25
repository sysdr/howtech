const express = require('express');
const session = require('express-session');
const cors = require('cors');
const { v4: uuidv4 } = require('uuid');

const app = express();
const PORT = process.env.PORT || 3000;
const SERVER_ID = process.env.SERVER_ID || `server-${PORT}`;

// In-memory session store (the problematic approach)
const MemoryStore = require('express-session').MemoryStore;

app.use(cors());
app.use(express.json());

app.use(session({
    genid: () => uuidv4(),
    secret: 'sticky-demo-secret',
    store: new MemoryStore(), // This is the problem!
    resave: false,
    saveUninitialized: false,
    cookie: { secure: false, maxAge: 1800000 } // 30 minutes
}));

// Shopping cart endpoints
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
    
    console.log(`[${SERVER_ID}] Cart updated for session ${req.sessionID}:`, req.session.cart);
    
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
    
    console.log(`[${SERVER_ID}] Cart retrieved for session ${req.sessionID}`);
    
    res.json({
        server: SERVER_ID,
        sessionId: req.sessionID,
        cart,
        cartTotal
    });
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ 
        status: 'healthy', 
        server: SERVER_ID,
        timestamp: new Date().toISOString(),
        memoryUsage: process.memoryUsage()
    });
});

// Simulate server crash
app.post('/crash', (req, res) => {
    console.log(`[${SERVER_ID}] Simulating crash...`);
    res.json({ message: `${SERVER_ID} going down!` });
    setTimeout(() => process.exit(1), 100);
});

app.listen(PORT, () => {
    console.log(`🏪 Sticky server ${SERVER_ID} running on port ${PORT}`);
    console.log(`⚠️  WARNING: Using in-memory session store - data will be lost on crash!`);
});
