const express = require('express');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app = express();
const PORT = 8080;

// Server pool
const servers = [
    'http://localhost:3001',
    'http://localhost:3002',
    'http://localhost:3003'
];

let currentServerIndex = 0;
const stickyMap = new Map(); // For sticky session routing

// Sticky session load balancer
app.use('/sticky/*', (req, res, next) => {
    const sessionId = req.headers['x-session-id'] || req.ip;
    
    // Route to same server if already mapped
    if (stickyMap.has(sessionId)) {
        const serverIndex = stickyMap.get(sessionId);
        req.targetServer = servers[serverIndex];
        console.log(`Routing session ${sessionId} to sticky server ${serverIndex}`);
    } else {
        // Assign to next server in rotation
        stickyMap.set(sessionId, currentServerIndex);
        req.targetServer = servers[currentServerIndex];
        console.log(`New session ${sessionId} assigned to server ${currentServerIndex}`);
        currentServerIndex = (currentServerIndex + 1) % servers.length;
    }
    next();
});

// Round-robin load balancer for stateless
app.use('/stateless/*', (req, res, next) => {
    req.targetServer = servers[currentServerIndex];
    console.log(`Round-robin routing to server ${currentServerIndex}`);
    currentServerIndex = (currentServerIndex + 1) % servers.length;
    next();
});

// Proxy middleware
app.use('*', (req, res) => {
    const target = req.targetServer || servers[0];
    createProxyMiddleware({
        target,
        changeOrigin: true,
        pathRewrite: {
            '^/sticky': '',
            '^/stateless': ''
        }
    })(req, res);
});

app.listen(PORT, () => {
    console.log(`🔀 Load Balancer running on port ${PORT}`);
    console.log(`Routing to servers: ${servers.join(', ')}`);
});
