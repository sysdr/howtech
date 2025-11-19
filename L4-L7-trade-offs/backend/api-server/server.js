const http = require('http');

const PORT = process.env.PORT || 4001;
const SERVER_ID = process.env.SERVER_ID || 'api-1';
const SERVICE_TYPE = process.env.SERVICE_TYPE || 'users';

let requestCount = 0;

// Simulated data
const mockData = {
    users: [
        { id: 1, name: 'Alice', email: 'alice@example.com' },
        { id: 2, name: 'Bob', email: 'bob@example.com' }
    ],
    orders: [
        { id: 101, userId: 1, total: 99.99, status: 'shipped' },
        { id: 102, userId: 2, total: 149.99, status: 'pending' }
    ],
    products: [
        { id: 1001, name: 'Widget', price: 29.99, stock: 100 },
        { id: 1002, name: 'Gadget', price: 49.99, stock: 50 }
    ]
};

const server = http.createServer((req, res) => {
    const start = process.hrtime.bigint();
    requestCount++;
    
    res.setHeader('X-Server-ID', SERVER_ID);
    res.setHeader('X-Service-Type', SERVICE_TYPE);
    
    // Handle CORS preflight requests
    if (req.method === 'OPTIONS') {
        res.writeHead(200, {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end();
        return;
    }
    
    if (req.url === '/health') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'healthy', server: SERVER_ID, service: SERVICE_TYPE }));
        return;
    }
    
    if (req.url === '/metrics') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            server: SERVER_ID,
            service: SERVICE_TYPE,
            requests: requestCount,
            uptime: process.uptime()
        }));
        return;
    }
    
    // Route based on service type
    const data = mockData[SERVICE_TYPE] || [];
    
    // Simulate some processing time (1-5ms)
    const delay = Math.random() * 4 + 1;
    setTimeout(() => {
        res.writeHead(200, { 
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end(JSON.stringify({
            service: SERVICE_TYPE,
            server: SERVER_ID,
            data: data,
            timestamp: new Date().toISOString()
        }));
        
        const end = process.hrtime.bigint();
        const latencyMs = Number(end - start) / 1000000;
        console.log(`[${SERVER_ID}] ${SERVICE_TYPE} request processed in ${latencyMs.toFixed(2)}ms`);
    }, delay);
});

server.listen(PORT, () => {
    console.log(`API server ${SERVER_ID} (${SERVICE_TYPE}) running on port ${PORT}`);
});
