# L4 vs L7 Load Balancer Trade-offs Demo

This demo compares Layer 4 (TCP) and Layer 7 (HTTP) load balancers in different scenarios:

- **L4 Load Balancer**: Optimized for video streaming with minimal latency
- **L7 Load Balancer**: Optimized for API gateway with URL-based routing

## Quick Start

1. Run the setup script:
   ```bash
   ./demo.sh
   ```

2. Open the dashboard:
   http://localhost:3000

3. Test the load balancers:
   - L4 (Video): `curl http://localhost:8080/chunk/1`
   - L7 (API): `curl http://localhost:8081/users`

4. Clean up:
   ```bash
   ./cleanup.sh
   ```

## Architecture

- **Backend Servers**: Node.js servers simulating video streaming and API microservices
- **Load Balancers**: HAProxy configured for L4 (TCP) and L7 (HTTP) modes
- **Dashboard**: Real-time monitoring and testing interface

## Files Generated

- `backend/video-server/server.js` - Video streaming server
- `backend/api-server/server.js` - API microservice server
- `haproxy/haproxy-l4.cfg` - L4 load balancer configuration
- `haproxy/haproxy-l7.cfg` - L7 load balancer configuration
- `dashboard/index.html` - Web dashboard
- `docker-compose.yml` - Docker orchestration
- `cleanup.sh` - Cleanup script
