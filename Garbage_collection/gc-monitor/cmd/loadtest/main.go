package main

import (
	"fmt"
	"log"
	"math/rand"
	"runtime"
	"time"
)

func main() {
	log.Println("🔥 Starting AGGRESSIVE GC pressure load test...")
	log.Println("This will create extreme memory pressure to reach YELLOW, ORANGE, and RED threat levels")
	log.Println("Targeting: >60% heap (YELLOW), >75% heap (ORANGE), >90% heap (RED)")

	// Create multiple data structures to maximize memory pressure
	var data [][]byte
	var strings []string
	var maps []map[string]interface{}

	// Phase 1: Build up to YELLOW (60% heap utilization)
	log.Println("🟡 Phase 1: Building to YELLOW threat level...")
	for i := 0; i < 200; i++ {
		// Allocate larger chunks (5MB to 50MB) for faster heap growth
		size := rand.Intn(45*1024*1024) + 5*1024*1024
		chunk := make([]byte, size)
		
		// Fill with data to prevent optimization
		for j := range chunk {
			chunk[j] = byte(rand.Intn(256))
		}
		data = append(data, chunk)

		// Also create string allocations
		str := make([]byte, 1024*1024) // 1MB strings
		for j := range str {
			str[j] = byte(rand.Intn(256))
		}
		strings = append(strings, string(str))

		if i%20 == 0 {
			var ms runtime.MemStats
			runtime.ReadMemStats(&ms)
			heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
			fmt.Printf("Phase 1 - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d\n",
				i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
			
			if heapUtil > 60 {
				log.Println("🟡 YELLOW threat level reached!")
				break
			}
		}
		time.Sleep(5 * time.Millisecond) // Faster allocation
	}

	// Phase 2: Push to ORANGE (75% heap utilization)
	log.Println("🟠 Phase 2: Pushing to ORANGE threat level...")
	for i := 0; i < 300; i++ {
		// Even larger allocations (10MB to 100MB)
		size := rand.Intn(90*1024*1024) + 10*1024*1024
		chunk := make([]byte, size)
		
		for j := range chunk {
			chunk[j] = byte(rand.Intn(256))
		}
		data = append(data, chunk)

		// Create map structures
		m := make(map[string]interface{})
		for k := 0; k < 10000; k++ {
			key := fmt.Sprintf("key_%d_%d", i, k)
			value := make([]byte, 1024)
			for l := range value {
				value[l] = byte(rand.Intn(256))
			}
			m[key] = value
		}
		maps = append(maps, m)

		if i%25 == 0 {
			var ms runtime.MemStats
			runtime.ReadMemStats(&ms)
			heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
			fmt.Printf("Phase 2 - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d\n",
				i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
			
			if heapUtil > 75 {
				log.Println("🟠 ORANGE threat level reached!")
				break
			}
		}
		time.Sleep(3 * time.Millisecond) // Even faster allocation
	}

	// Phase 3: Force RED (90% heap utilization)
	log.Println("🔴 Phase 3: Forcing RED threat level...")
	for i := 0; i < 500; i++ {
		// Massive allocations (50MB to 200MB)
		size := rand.Intn(150*1024*1024) + 50*1024*1024
		chunk := make([]byte, size)
		
		for j := range chunk {
			chunk[j] = byte(rand.Intn(256))
		}
		data = append(data, chunk)

		// Create nested structures
		nested := make([]map[string][]byte, 1000)
		for k := range nested {
			nested[k] = make(map[string][]byte)
			for l := 0; l < 100; l++ {
				key := fmt.Sprintf("nested_%d_%d", k, l)
				value := make([]byte, 10240) // 10KB values
				for m := range value {
					value[m] = byte(rand.Intn(256))
				}
				nested[k][key] = value
			}
		}

		if i%30 == 0 {
			var ms runtime.MemStats
			runtime.ReadMemStats(&ms)
			heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
			fmt.Printf("Phase 3 - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d\n",
				i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
			
			if heapUtil > 90 {
				log.Println("🔴 RED threat level reached!")
				break
			}
		}
		time.Sleep(1 * time.Millisecond) // Maximum speed
	}

	// Phase 4: Hold the pressure for a bit to observe responses
	log.Println("⏳ Phase 4: Holding pressure to observe system response...")
	time.Sleep(10 * time.Second)

	// Phase 5: Gradual cleanup to see recovery
	log.Println("🧹 Phase 5: Gradual cleanup and recovery...")
	for i := 0; i < len(data); i += 10 {
		if i+10 < len(data) {
			data[i] = nil
		}
		if i < len(strings) {
			strings[i] = ""
		}
		if i < len(maps) {
			maps[i] = nil
		}
		
		if i%100 == 0 {
			var ms runtime.MemStats
			runtime.ReadMemStats(&ms)
			heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
			fmt.Printf("Cleanup - Iteration %d: Heap=%dMB (%.1f%%), Objects=%d, GC=%d\n",
				i, ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
		}
		time.Sleep(50 * time.Millisecond)
	}

	// Final cleanup
	data = nil
	strings = nil
	maps = nil
	runtime.GC()
	runtime.GC()

	log.Println("✅ AGGRESSIVE load test completed!")
	log.Println("📊 Final stats:")
	var ms runtime.MemStats
	runtime.ReadMemStats(&ms)
	heapUtil := float64(ms.HeapAlloc) / float64(ms.HeapSys) * 100
	fmt.Printf("Final: Heap=%dMB (%.1f%%), Objects=%d, Total GC=%d\n",
		ms.HeapAlloc/1024/1024, heapUtil, ms.HeapObjects, ms.NumGC)
}
