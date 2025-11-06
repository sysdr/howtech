const { useState, useEffect } = React;

function Dashboard() {
    const [requests, setRequests] = useState([]);
    const [stats, setStats] = useState({ total: 0, cached: 0, rateLimited: 0 });
    const [loading, setLoading] = useState(false);

    const sendRequest = async () => {
        setLoading(true);
        const startTime = Date.now();
        
        try {
            const response = await fetch('http://localhost:8080/api/users', {
                headers: { 'apikey': 'demo-api-key-12345' }
            });
            
            const endTime = Date.now();
            const data = await response.json();
            
            const newRequest = {
                id: Date.now(),
                timestamp: new Date().toISOString(),
                status: response.status,
                latency: endTime - startTime,
                cached: response.headers.get('x-cache-status') === 'HIT',
                rateLimited: response.status === 429,
                headers: {
                    loadBalancer: response.headers.get('x-load-balancer'),
                    backend: response.headers.get('x-lb-backend'),
                    proxy: response.headers.get('x-reverse-proxy'),
                    cache: response.headers.get('x-cache-status')
                },
                data: data
            };
            
            setRequests(prev => [newRequest, ...prev].slice(0, 20));
            setStats(prev => ({
                total: prev.total + 1,
                cached: prev.cached + (newRequest.cached ? 1 : 0),
                rateLimited: prev.rateLimited + (newRequest.rateLimited ? 1 : 0)
            }));
        } catch (error) {
            console.error('Request failed:', error);
        }
        
        setLoading(false);
    };

    const sendBurst = async () => {
        for (let i = 0; i < 15; i++) {
            await sendRequest();
            await new Promise(r => setTimeout(r, 100));
        }
    };

    return (
        <div className="container mx-auto px-4 py-8 bg-black">
            {/* Header */}
            <div className="text-center mb-12">
                <h1 className="text-5xl font-bold text-white mb-4">
                    🔄 Proxy Showdown Dashboard
                </h1>
                <p className="text-gray-400 text-lg">
                    Watch requests flow through Load Balancer → API Gateway → Reverse Proxy → Backend
                </p>
            </div>

            {/* Stats Cards */}
            <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
                <div className="bg-gray-900 rounded-xl p-6 border border-gray-800 shadow-lg">
                    <div className="text-blue-400 text-sm font-semibold mb-2">TOTAL REQUESTS</div>
                    <div className="text-4xl font-bold text-white">{stats.total}</div>
                </div>
                <div className="bg-gray-900 rounded-xl p-6 border border-gray-800 shadow-lg">
                    <div className="text-green-400 text-sm font-semibold mb-2">CACHE HITS</div>
                    <div className="text-4xl font-bold text-white">
                        {stats.cached}
                        <span className="text-lg text-green-400 ml-2">
                            ({stats.total > 0 ? Math.round(stats.cached/stats.total*100) : 0}%)
                        </span>
                    </div>
                </div>
                <div className="bg-gray-900 rounded-xl p-6 border border-gray-800 shadow-lg">
                    <div className="text-red-400 text-sm font-semibold mb-2">RATE LIMITED</div>
                    <div className="text-4xl font-bold text-white">{stats.rateLimited}</div>
                </div>
            </div>

            {/* Action Buttons */}
            <div className="flex gap-4 mb-8 justify-center">
                <button
                    onClick={sendRequest}
                    disabled={loading}
                    className="bg-blue-600 hover:bg-blue-700 disabled:bg-gray-800 disabled:text-gray-500 text-white font-bold py-4 px-8 rounded-lg shadow-lg transition-all transform hover:scale-105 border border-blue-500"
                >
                    {loading ? '⏳ Sending...' : '📤 Send Request'}
                </button>
                <button
                    onClick={sendBurst}
                    disabled={loading}
                    className="bg-purple-600 hover:bg-purple-700 disabled:bg-gray-800 disabled:text-gray-500 text-white font-bold py-4 px-8 rounded-lg shadow-lg transition-all transform hover:scale-105 border border-purple-500"
                >
                    💥 Send Burst (15 req) - See Rate Limiting!
                </button>
            </div>

            {/* Request Log */}
            <div className="bg-gray-900 rounded-xl p-6 border border-gray-800 shadow-lg">
                <h2 className="text-2xl font-bold text-white mb-4">📊 Request Flow</h2>
                <div className="space-y-3 max-h-96 overflow-y-auto">
                    {requests.length === 0 ? (
                        <div className="text-center text-gray-400 py-8">
                            Click "Send Request" to see the magic happen ✨
                        </div>
                    ) : (
                        requests.map(req => (
                            <div
                                key={req.id}
                                className={`p-4 rounded-lg border-l-4 ${
                                    req.rateLimited
                                        ? 'bg-gray-800 border-red-500'
                                        : req.cached
                                        ? 'bg-gray-800 border-green-500'
                                        : 'bg-gray-800 border-blue-500'
                                }`}
                            >
                                <div className="flex justify-between items-start mb-2">
                                    <div className="flex items-center gap-2">
                                        <span className="text-2xl">
                                            {req.rateLimited ? '🚫' : req.cached ? '⚡' : '🔄'}
                                        </span>
                                        <span className="text-white font-mono text-sm">
                                            {new Date(req.timestamp).toLocaleTimeString()}
                                        </span>
                                    </div>
                                    <div className={`px-3 py-1 rounded-full text-sm font-semibold ${
                                        req.status === 200 ? 'bg-green-900/50 text-green-400 border border-green-700' : 'bg-red-900/50 text-red-400 border border-red-700'
                                    }`}>
                                        {req.status} • {req.latency}ms
                                    </div>
                                </div>
                                
                                <div className="grid grid-cols-4 gap-2 text-sm">
                                    <div className="bg-gray-800 p-2 rounded border border-gray-700">
                                        <div className="text-blue-400 text-xs font-semibold">Load Balancer</div>
                                        <div className="text-white font-semibold">{req.headers.loadBalancer}</div>
                                        <div className="text-gray-400 text-xs">{req.headers.backend}</div>
                                    </div>
                                    <div className="bg-gray-800 p-2 rounded border border-gray-700">
                                        <div className="text-green-400 text-xs font-semibold">API Gateway</div>
                                        <div className="text-white font-semibold">Kong</div>
                                        <div className="text-gray-400 text-xs">Auth ✓</div>
                                    </div>
                                    <div className="bg-gray-800 p-2 rounded border border-gray-700">
                                        <div className="text-purple-400 text-xs font-semibold">Reverse Proxy</div>
                                        <div className="text-white font-semibold">{req.headers.proxy}</div>
                                        <div className="text-gray-400 text-xs">{req.headers.cache}</div>
                                    </div>
                                    <div className="bg-gray-800 p-2 rounded border border-gray-700">
                                        <div className="text-orange-400 text-xs font-semibold">Backend</div>
                                        <div className="text-white font-semibold">{req.data.instance}</div>
                                        <div className="text-gray-400 text-xs">Node.js</div>
                                    </div>
                                </div>
                            </div>
                        ))
                    )}
                </div>
            </div>

            {/* Legend */}
            <div className="mt-8 bg-gray-900 rounded-xl p-6 border border-gray-800 shadow-lg">
                <h3 className="text-xl font-bold text-white mb-4">🎯 What You're Seeing</h3>
                <div className="grid grid-cols-1 md:grid-cols-3 gap-4 text-sm">
                    <div className="bg-gray-800 p-4 rounded-lg border border-gray-700">
                        <span className="text-2xl mr-2">⚡</span>
                        <span className="text-green-400 font-semibold">CACHE HIT:</span>
                        <span className="text-gray-300 block mt-1"> NGINX returned cached response. Notice &lt;1ms latency!</span>
                    </div>
                    <div className="bg-gray-800 p-4 rounded-lg border border-gray-700">
                        <span className="text-2xl mr-2">🔄</span>
                        <span className="text-blue-400 font-semibold">CACHE MISS:</span>
                        <span className="text-gray-300 block mt-1"> Request went through all layers to backend. ~50-100ms latency.</span>
                    </div>
                    <div className="bg-gray-800 p-4 rounded-lg border border-gray-700">
                        <span className="text-2xl mr-2">🚫</span>
                        <span className="text-red-400 font-semibold">RATE LIMITED:</span>
                        <span className="text-gray-300 block mt-1"> Kong blocked request (10/min limit). Try the burst!</span>
                    </div>
                </div>
            </div>
        </div>
    );
}

ReactDOM.render(<Dashboard />, document.getElementById('root'));
