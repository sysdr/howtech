const express = require('express');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app = express();
const PORT = 8080;

// Simple round-robin load balancer
let currentServer = 0;
const servers = [
  'http://localhost:3001',
  'http://localhost:3002'
];

const stickyProxy = createProxyMiddleware({
  target: 'http://localhost:3001', // Default target
  changeOrigin: true,
  router: (req) => {
    // Sticky session routing based on session cookie
    const sessionCookie = req.headers.cookie;
    if (sessionCookie && sessionCookie.includes('connect.sid')) {
      // Extract session ID and hash to server
      const sessionId = sessionCookie.match(/connect\.sid=([^;]*)/)?.[1];
      if (sessionId) {
        const serverIndex = Math.abs(sessionId.hashCode()) % servers.length;
        console.log(`Routing session ${sessionId.substring(0, 8)}... to server ${serverIndex + 1}`);
        return servers[serverIndex];
      }
    }
    
    // New session - use round robin
    const server = servers[currentServer];
    currentServer = (currentServer + 1) % servers.length;
    console.log(`New session routed to server ${currentServer}`);
    return server;
  }
});

// Add hash function to String prototype
String.prototype.hashCode = function() {
  let hash = 0;
  for (let i = 0; i < this.length; i++) {
    const char = this.charCodeAt(i);
    hash = ((hash << 5) - hash) + char;
    hash = hash & hash; // Convert to 32-bit integer
  }
  return hash;
};

app.use('/', stickyProxy);

app.listen(PORT, () => {
  console.log(`Load balancer running on port ${PORT}`);
  console.log(`Routing between: ${servers.join(', ')}`);
});
