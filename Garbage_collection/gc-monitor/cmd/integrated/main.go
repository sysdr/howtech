package main

import (
	"encoding/json"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"runtime"
	"time"
)

type GCMetrics struct {
	Timestamp      int64   `json:"timestamp"`
	HeapSize       uint64  `json:"heap_size"`
	HeapUsed       uint64  `json:"heap_used"`
	HeapObjects    uint64  `json:"heap_objects"`
	GCPauses       []int64 `json:"gc_pauses"`
	NextGC         uint64  `json:"next_gc"`
	AllocationRate float64 `json:"allocation_rate"`
	ThreatLevel    string  `json:"threat_level"`
}

type Monitor struct {
	metrics       []GCMetrics
	lastHeapUsed  uint64
	lastTimestamp int64
	loadTestData  [][]byte // Data for load testing
	loadTestActive bool
}

func (m *Monitor) collectMetrics() GCMetrics {
	var ms runtime.MemStats
	runtime.ReadMemStats(&ms)

	now := time.Now().UnixMilli()

	// Calculate allocation rate (bytes per second)
	var allocRate float64
	if m.lastTimestamp > 0 {
		timeDiff := float64(now-m.lastTimestamp) / 1000.0
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

func (m *Monitor) startLoadTest() {
	log.Println("🔥 Starting integrated load test...")
	m.loadTestActive = true

	go func() {
		// Phase 1: Build to YELLOW
		log.Println("🟡 Phase 1: Building to YELLOW threat level...")
		for i := 0; i < 50; i++ {
			size := rand.Intn(10*1024*1024) + 1024*1024 // 1MB to 10MB
			chunk := make([]byte, size)
			
			for j := range chunk {
				chunk[j] = byte(rand.Intn(256))
			}
			m.loadTestData = append(m.loadTestData, chunk)

			if i%10 == 0 {
				var ms runtime.MemStats
				runtime.ReadMemStats(&ms)
				heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
				log.Printf("Phase 1 - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d",
					i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
				
				if heapUtil > 60 {
					log.Println("🟡 YELLOW threat level reached!")
					break
				}
			}
			time.Sleep(100 * time.Millisecond)
		}

		// Phase 2: Push to ORANGE
		log.Println("🟠 Phase 2: Pushing to ORANGE threat level...")
		for i := 0; i < 100; i++ {
			size := rand.Intn(20*1024*1024) + 10*1024*1024 // 10MB to 30MB
			chunk := make([]byte, size)
			
			for j := range chunk {
				chunk[j] = byte(rand.Intn(256))
			}
			m.loadTestData = append(m.loadTestData, chunk)

			if i%20 == 0 {
				var ms runtime.MemStats
				runtime.ReadMemStats(&ms)
				heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
				log.Printf("Phase 2 - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d",
					i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
				
				if heapUtil > 75 {
					log.Println("🟠 ORANGE threat level reached!")
					break
				}
			}
			time.Sleep(50 * time.Millisecond)
		}

		// Phase 3: Force RED
		log.Println("🔴 Phase 3: Forcing RED threat level...")
		for i := 0; i < 200; i++ {
			size := rand.Intn(50*1024*1024) + 20*1024*1024 // 20MB to 70MB
			chunk := make([]byte, size)
			
			for j := range chunk {
				chunk[j] = byte(rand.Intn(256))
			}
			m.loadTestData = append(m.loadTestData, chunk)

			if i%30 == 0 {
				var ms runtime.MemStats
				runtime.ReadMemStats(&ms)
				heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
				log.Printf("Phase 3 - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d",
					i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
				
				if heapUtil > 90 {
					log.Println("🔴 RED threat level reached!")
					break
				}
			}
			time.Sleep(25 * time.Millisecond)
		}

		// Hold pressure for a bit
		log.Println("⏳ Holding pressure to observe system response...")
		time.Sleep(10 * time.Second)

		// Cleanup
		log.Println("🧹 Cleaning up load test data...")
		for i := range m.loadTestData {
			m.loadTestData[i] = nil
		}
		m.loadTestData = nil
		runtime.GC()
		runtime.GC()
		m.loadTestActive = false

		log.Println("✅ Integrated load test completed!")
	}()
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
		fmt.Fprintf(w, "Integrated GC Monitor is running")
	})

	http.HandleFunc("/loadtest", func(w http.ResponseWriter, r *http.Request) {
		if monitor.loadTestActive {
			http.Error(w, "Load test already running", http.StatusConflict)
			return
		}
		monitor.startLoadTest()
		w.WriteHeader(http.StatusOK)
		fmt.Fprintf(w, "Load test started")
	})

	log.Println("🔍 Integrated GC Monitor started on :8080")
	log.Println("📊 Metrics endpoint: http://localhost:8080/metrics")
	log.Println("🔥 Load test endpoint: http://localhost:8080/loadtest")
	log.Println("💡 Use: curl http://localhost:8080/loadtest to start load test")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
