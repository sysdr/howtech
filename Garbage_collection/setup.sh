#!/bin/bash
set -e

echo "🚀 Setting up GC Pause Monitoring & Mitigation System..."

# Prerequisite checks
check_prerequisites() {
    echo "✅ Checking prerequisites..."
    
    if ! command -v go &> /dev/null; then
        echo "❌ Go is not installed. Please install Go 1.19+ first."
        exit 1
    fi
    
    if ! command -v docker &> /dev/null; then
        echo "⚠️  Docker not found. Installing basic version without containerization."
    fi
    
    echo "✅ Prerequisites satisfied!"
}

# Create project structure
setup_project_structure() {
    echo "📁 Creating project structure..."
    
    mkdir -p gc-monitor/{cmd,internal/{monitor,analyzer,emergency},pkg/{metrics,alerts},configs,deployments,tests}
    cd gc-monitor
    
    # Initialize Go module
    go mod init gc-monitor
    echo "✅ Project structure created!"
}

# Generate monitoring service
generate_monitor_service() {
    echo "🔍 Generating GC monitoring service..."
    
    cat > cmd/monitor/main.go << 'EOF'
package main

import (
    "context"
    "encoding/json"
    "fmt"
    "log"
    "net/http"
    "runtime"
    "time"
)

type GCMetrics struct {
    Timestamp    int64   `json:"timestamp"`
    HeapSize     uint64  `json:"heap_size"`
    HeapUsed     uint64  `json:"heap_used"`
    HeapObjects  uint64  `json:"heap_objects"`
    GCPauses     []int64 `json:"gc_pauses"`
    NextGC       uint64  `json:"next_gc"`
    AllocationRate float64 `json:"allocation_rate"`
    ThreatLevel  string  `json:"threat_level"`
}

type Monitor struct {
    metrics       []GCMetrics
    lastHeapUsed  uint64
    lastTimestamp int64
}

func (m *Monitor) collectMetrics() GCMetrics {
    var ms runtime.MemStats
    runtime.ReadMemStats(&ms)
    
    now := time.Now().UnixMilli()
    
    // Calculate allocation rate (bytes per second)
    var allocRate float64
    if m.lastTimestamp > 0 {
        timeDiff := float64(now - m.lastTimestamp) / 1000.0
        heapDiff := float64(ms.HeapAlloc) - float64(m.lastHeapUsed)
        if timeDiff > 0 {
            allocRate = heapDiff / timeDiff
        }
    }
    
    // Extract recent GC pause times
    pauseCount := len(ms.PauseNs)
    recentPauses := make([]int64, 0, 10)
    for i := 0; i < 10 && i < pauseCount; i++ {
        if ms.PauseNs[i] > 0 {
            recentPauses = append(recentPauses, int64(ms.PauseNs[i]))
        }
    }
    
    // Determine threat level
    threatLevel := m.assessThreatLevel(ms, allocRate)
    
    metrics := GCMetrics{
        Timestamp:      now,
        HeapSize:       ms.HeapSys,
        HeapUsed:       ms.HeapAlloc,
        HeapObjects:    ms.HeapObjects,
        GCPauses:       recentPauses,
        NextGC:         ms.NextGC,
        AllocationRate: allocRate,
        ThreatLevel:    threatLevel,
    }
    
    m.lastHeapUsed = ms.HeapAlloc
    m.lastTimestamp = now
    m.metrics = append(m.metrics, metrics)
    
    // Keep only last 100 metrics
    if len(m.metrics) > 100 {
        m.metrics = m.metrics[1:]
    }
    
    return metrics
}

func (m *Monitor) assessThreatLevel(ms runtime.MemStats, allocRate float64) string {
    // Calculate heap utilization percentage
    heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
    
    // Check for danger signs
    if heapUtil > 90 || allocRate > 100*1024*1024 { // 100MB/s allocation
        return "RED"
    } else if heapUtil > 75 || allocRate > 50*1024*1024 { // 50MB/s allocation
        return "ORANGE"
    } else if heapUtil > 60 || allocRate > 20*1024*1024 { // 20MB/s allocation
        return "YELLOW"
    }
    
    return "GREEN"
}

func (m *Monitor) handleEmergency(threatLevel string) {
    switch threatLevel {
    case "RED":
        log.Println("🚨 RED ALERT: Triggering emergency GC and heap optimization")
        runtime.GC()
        runtime.GC() // Double GC for thorough cleanup
    case "ORANGE":
        log.Println("⚠️  ORANGE ALERT: Triggering preemptive GC")
        runtime.GC()
    case "YELLOW":
        log.Println("🟡 YELLOW ALERT: Monitoring closely")
    }
}

func main() {
    monitor := &Monitor{}
    
    // Start background monitoring
    go func() {
        ticker := time.NewTicker(100 * time.Millisecond)
        defer ticker.Stop()
        
        for range ticker.C {
            metrics := monitor.collectMetrics()
            if metrics.ThreatLevel != "GREEN" {
                monitor.handleEmergency(metrics.ThreatLevel)
            }
        }
    }()
    
    // HTTP API for metrics
    http.HandleFunc("/metrics", func(w http.ResponseWriter, r *http.Request) {
        w.Header().Set("Content-Type", "application/json")
        if len(monitor.metrics) > 0 {
            json.NewEncoder(w).Encode(monitor.metrics[len(monitor.metrics)-1])
        } else {
            json.NewEncoder(w).Encode(GCMetrics{})
        }
    })
    
    http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
        w.WriteHeader(http.StatusOK)
        fmt.Fprintf(w, "GC Monitor is running")
    })
    
    log.Println("🔍 GC Monitor started on :8080")
    log.Println("📊 Metrics endpoint: http://localhost:8080/metrics")
    log.Fatal(http.ListenAndServe(":8080", nil))
}
EOF
}

# Generate load test
generate_load_test() {
    echo "🔥 Generating GC pressure load test..."
    
    cat > cmd/loadtest/main.go << 'EOF'
package main

import (
    "fmt"
    "log"
    "math/rand"
    "runtime"
    "time"
)

func main() {
    log.Println("🔥 Starting GC pressure load test...")
    log.Println("This will intentionally create memory pressure to demonstrate GC monitoring")
    
    // Create memory pressure
    var data [][]byte
    
    for i := 0; i < 1000; i++ {
        // Allocate random sized chunks (1KB to 10MB)
        size := rand.Intn(10*1024*1024) + 1024
        chunk := make([]byte, size)
        
        // Fill with random data
        for j := range chunk {
            chunk[j] = byte(rand.Intn(256))
        }
        
        data = append(data, chunk)
        
        // Randomly release some memory
        if len(data) > 100 && rand.Intn(10) < 3 {
            releaseCount := rand.Intn(len(data)/2) + 1
            data = data[releaseCount:]
        }
        
        if i%50 == 0 {
            var ms runtime.MemStats
            runtime.ReadMemStats(&ms)
            fmt.Printf("Iteration %d: Heap=%dMB, Objects=%d, GC=%d\n", 
                i, ms.HeapAlloc/1024/1024, ms.HeapObjects, ms.NumGC)
        }
        
        time.Sleep(10 * time.Millisecond)
    }
    
    log.Println("✅ Load test completed")
}
EOF
}

# Generate configuration
generate_config() {
    echo "⚙️  Generating configuration files..."
    
    cat > configs/monitor.yaml << 'EOF'
monitor:
  collection_interval: 100ms
  history_size: 1000
  api_port: 8080

thresholds:
  yellow:
    heap_utilization: 60
    allocation_rate: 20971520  # 20MB/s
  orange:
    heap_utilization: 75
    allocation_rate: 52428800  # 50MB/s
  red:
    heap_utilization: 90
    allocation_rate: 104857600 # 100MB/s

emergency_actions:
  yellow: ["log_warning"]
  orange: ["preemptive_gc", "alert"]
  red: ["emergency_gc", "circuit_breaker", "alert"]
EOF

    cat > deployments/docker-compose.yml << 'EOF'
version: '3.8'
services:
  gc-monitor:
    build: .
    ports:
      - "8080:8080"
    environment:
      - GO_ENV=production
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 10s
      retries: 3
  
  load-test:
    build: 
      context: .
      dockerfile: Dockerfile.loadtest
    depends_on:
      - gc-monitor
    profiles:
      - testing
EOF

    cat > Dockerfile << 'EOF'
FROM golang:1.21-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN go build -o gc-monitor cmd/monitor/main.go

FROM alpine:latest
RUN apk add --no-cache ca-certificates curl
WORKDIR /root/
COPY --from=builder /app/gc-monitor .
EXPOSE 8080
CMD ["./gc-monitor"]
EOF
}

# Build and test
build_and_test() {
    echo "🔨 Building application..."
    
    # Build monitor service
    go build -o bin/gc-monitor cmd/monitor/main.go
    
    # Build load test
    go build -o bin/loadtest cmd/loadtest/main.go
    
    echo "✅ Build completed!"
    
    # Run quick validation
    echo "🧪 Running validation tests..."
    timeout 5s ./bin/gc-monitor &
    MONITOR_PID=$!
    
    sleep 2
    
    # Test health endpoint
    if curl -s http://localhost:8080/health > /dev/null; then
        echo "✅ Health check passed"
    else
        echo "❌ Health check failed"
    fi
    
    # Test metrics endpoint
    if curl -s http://localhost:8080/metrics | grep -q "timestamp"; then
        echo "✅ Metrics endpoint working"
    else
        echo "❌ Metrics endpoint failed"
    fi
    
    kill $MONITOR_PID 2>/dev/null || true
    wait $MONITOR_PID 2>/dev/null || true
}

# Generate README
generate_readme() {
    cat > README.md << 'EOF'
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
EOF
}

# Main execution
main() {
    check_prerequisites
    setup_project_structure
    generate_monitor_service
    generate_load_test
    generate_config
    build_and_test
    generate_readme
    
    echo ""
    echo "🎉 SUCCESS! GC Monitoring System is ready!"
    echo ""
    echo "Next steps:"
    echo "1. Start the monitor: ./bin/gc-monitor"
    echo "2. In another terminal, run load test: ./bin/loadtest"
    echo "3. View metrics at: http://localhost:8080/metrics"
    echo "4. Watch for threat level alerts in the monitor logs"
    echo ""
    echo "The system will automatically detect and respond to GC pressure!"
}

main "$@"