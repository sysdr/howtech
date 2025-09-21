# GC Pause Monitoring & Mitigation System

A production-ready system for detecting and preventing garbage collection pauses before they cause service outages.

## Quick Start

```bash
# Start the monitor
./bin/gc-monitor

# In another terminal, run load test
./bin/loadtest

# Check metrics
curl http://localhost:8080/metrics

# Monitor threat levels in real-time
./monitor_threats.sh
# OR use the simpler version
./simple_monitor.sh
```

## Threat Levels

- **GREEN**: Normal operation
- **YELLOW**: Elevated memory pressure (60%+ heap utilization)
- **ORANGE**: High risk of GC pause (75%+ heap utilization)
- **RED**: Emergency intervention required (90%+ heap utilization)

## Production Deployment

Use the provided Docker Compose configuration for containerized deployment:

```bash
docker-compose up --build
```

For production monitoring, integrate with your observability stack (Prometheus, DataDog, etc.) using the `/metrics` endpoint.
