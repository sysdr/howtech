const express = require('express');
const app = express();
const port = 3000;

const instanceId = process.env.INSTANCE_ID || 'unknown';
const requestLog = [];

app.use(express.json());

// Middleware to log all requests
app.use((req, res, next) => {
    const logEntry = {
        timestamp: new Date().toISOString(),
        instance: instanceId,
        method: req.method,
        path: req.path,
        headers: {
            'x-load-balancer': req.get('x-load-balancer'),
            'x-lb-backend': req.get('x-lb-backend'),
            'x-request-id': req.get('x-request-id'),
            'x-reverse-proxy': req.get('x-reverse-proxy'),
            'x-cache-status': req.get('x-cache-status')
        }
    };
    requestLog.push(logEntry);
    if (requestLog.length > 100) requestLog.shift();
    console.log(`[${instanceId}] ${req.method} ${req.path}`);
    next();
});

app.get('/health', (req, res) => {
    res.json({ status: 'healthy', instance: instanceId });
});

app.get('/users', (req, res) => {
    // Simulate some processing time
    setTimeout(() => {
        res.json({
            instance: instanceId,
            timestamp: new Date().toISOString(),
            data: [
                { id: 1, name: 'Alice' },
                { id: 2, name: 'Bob' },
                { id: 3, name: 'Charlie' }
            ],
            headers: req.headers
        });
    }, 50);
});

app.get('/logs', (req, res) => {
    res.json({ logs: requestLog });
});

app.get('/metrics', (req, res) => {
    res.json({
        instance: instanceId,
        totalRequests: requestLog.length,
        uptime: process.uptime()
    });
});

app.listen(port, '0.0.0.0', () => {
    console.log(`Backend ${instanceId} listening on port ${port}`);
});
