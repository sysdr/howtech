const http = require('http');
const os = require('os');

const PORT = process.env.PORT || 3001;
const SERVER_ID = process.env.SERVER_ID || 'video-1';

// Simulate video chunk (100KB of data)
const VIDEO_CHUNK = Buffer.alloc(100 * 1024, 'V');

let requestCount = 0;
let totalBytes = 0;

const server = http.createServer((req, res) => {
    const start = process.hrtime.bigint();
    requestCount++;
    
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
        res.end(JSON.stringify({ status: 'healthy', server: SERVER_ID }));
        return;
    }
    
    if (req.url === '/metrics') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            server: SERVER_ID,
            requests: requestCount,
            bytesServed: totalBytes,
            uptime: process.uptime()
        }));
        return;
    }
    
    // Simulate video chunk delivery
    if (req.url.startsWith('/chunk')) {
        totalBytes += VIDEO_CHUNK.length;
        res.writeHead(200, {
            'Content-Type': 'video/mp4',
            'Content-Length': VIDEO_CHUNK.length,
            'X-Server-ID': SERVER_ID,
            'X-Processing-Time': '0',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, X-Server-ID'
        });
        res.end(VIDEO_CHUNK);
        
        const end = process.hrtime.bigint();
        const latencyUs = Number(end - start) / 1000;
        console.log(`[${SERVER_ID}] Chunk served in ${latencyUs.toFixed(0)}µs`);
        return;
    }
    
    res.writeHead(404);
    res.end('Not found');
});

server.listen(PORT, () => {
    console.log(`Video server ${SERVER_ID} running on port ${PORT}`);
});
