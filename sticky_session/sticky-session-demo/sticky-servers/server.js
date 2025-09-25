const express = require('express');
const session = require('express-session');

const app = express();
const PORT = process.env.PORT || 3001;
const SERVER_ID = process.env.SERVER_ID || 'server-1';

// In-memory session store (the problem!)
app.use(session({
  secret: 'sticky-session-secret',
  resave: false,
  saveUninitialized: true,
  cookie: { maxAge: 300000 } // 5 minutes
}));

app.use(express.json());

// Simulate shopping cart
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

// Simulate server crash
app.post('/crash', (req, res) => {
  console.log(`[${SERVER_ID}] CRASH INITIATED! All sessions will be lost...`);
  res.json({ message: `${SERVER_ID} is going down!` });
  setTimeout(() => {
    console.log(`[${SERVER_ID}] 💥 CRASHED - All session data lost!`);
    process.exit(1);
  }, 1000);
});

app.listen(PORT, () => {
  console.log(`[${SERVER_ID}] Sticky server running on port ${PORT}`);
});
