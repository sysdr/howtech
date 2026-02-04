#!/usr/bin/env python3
"""
Priority Inversion Dashboard
Displays real-time metrics from the priority inversion demonstration
"""

import json
import subprocess
import time
import sys
import os
import re
from collections import defaultdict
from datetime import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
METRICS_FILE = os.path.join(SCRIPT_DIR, "metrics.json")
DEMO_LOG = os.path.join(SCRIPT_DIR, "demo.log")

def parse_demo_output(output):
    """Parse output from priority_inversion program"""
    metrics = {
        "high_lock_requests": 0,
        "high_lock_acquired": 0,
        "avg_wait_time_ms": 0.0,
        "total_wait_time_ms": 0.0,
        "medium_cpu_cycles": 0,
        "use_pi": False,
        "timestamp": datetime.now().isoformat()
    }
    
    lines = output.split('\n')
    for i, line in enumerate(lines):
        if "Using Priority Inheritance (PI) Mutex" in line:
            metrics["use_pi"] = True
        elif "Using Regular Mutex (No PI)" in line:
            metrics["use_pi"] = False
        elif "Lock requests:" in line or "lock requests:" in line.lower():
            match = re.search(r'(\d+)', line)
            if match:
                metrics["high_lock_requests"] = int(match.group(1))
        elif "Lock acquired:" in line or "lock acquired:" in line.lower():
            match = re.search(r'(\d+)', line)
            if match:
                metrics["high_lock_acquired"] = int(match.group(1))
        elif "Average wait time:" in line:
            match = re.search(r'(\d+)\.(\d+)', line)
            if match:
                metrics["avg_wait_time_ms"] = float(f"{match.group(1)}.{match.group(2)}")
        elif "Total wait time:" in line:
            match = re.search(r'(\d+)\.(\d+)', line)
            if match:
                metrics["total_wait_time_ms"] = float(f"{match.group(1)}.{match.group(2)}")
        elif "Medium priority CPU cycles:" in line:
            match = re.search(r'(\d+)', line)
            if match:
                metrics["medium_cpu_cycles"] = int(match.group(1))
    
    return metrics

def get_process_metrics():
    """Get real-time process metrics from /proc"""
    metrics = {
        "processes": [],
        "timestamp": datetime.now().isoformat()
    }
    
    try:
        # Find priority_inversion processes
        result = subprocess.run(
            ["pgrep", "-f", "priority_inversion"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            pids = result.stdout.strip().split('\n')
            for pid in pids:
                if not pid:
                    continue
                try:
                    # Read /proc/[pid]/status
                    status_file = f"/proc/{pid}/status"
                    if os.path.exists(status_file):
                        proc_info = {"pid": int(pid)}
                        with open(status_file, 'r') as f:
                            for line in f:
                                if line.startswith("Name:"):
                                    proc_info["name"] = line.split()[1]
                                elif line.startswith("Threads:"):
                                    proc_info["threads"] = int(line.split()[1])
                        
                        # Read /proc/[pid]/sched
                        sched_file = f"/proc/{pid}/sched"
                        if os.path.exists(sched_file):
                            with open(sched_file, 'r') as f:
                                for line in f:
                                    if "prio" in line and ":" in line:
                                        match = re.search(r'prio\s*:\s*(\d+)', line)
                                        if match:
                                            proc_info["priority"] = int(match.group(1))
                        
                        metrics["processes"].append(proc_info)
                except (ValueError, FileNotFoundError, PermissionError):
                    continue
    except Exception as e:
        pass
    
    return metrics

def display_dashboard(metrics_history, process_metrics):
    """Display the dashboard"""
    os.system('clear' if os.name != 'nt' else 'cls')
    
    print("╔" + "═" * 78 + "╗")
    print("║" + " " * 20 + "Priority Inversion Dashboard" + " " * 30 + "║")
    print("╠" + "═" * 78 + "╣")
    print("║ " + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + " " * 55 + "║")
    print("╠" + "═" * 78 + "╣")
    
    if metrics_history:
        latest = metrics_history[-1]
        print("║ Latest Demo Results:" + " " * 57 + "║")
        print("╠" + "─" * 78 + "╣")
        print(f"║ Mode: {'Priority Inheritance (PI)' if latest.get('use_pi') else 'Regular Mutex (No PI)'}" + " " * 40 + "║")
        print(f"║ High Priority Lock Requests: {latest.get('high_lock_requests', 0)}" + " " * 40 + "║")
        print(f"║ High Priority Lock Acquired: {latest.get('high_lock_acquired', 0)}" + " " * 41 + "║")
        print(f"║ Average Wait Time: {latest.get('avg_wait_time_ms', 0.0):.3f} ms" + " " * 45 + "║")
        print(f"║ Total Wait Time: {latest.get('total_wait_time_ms', 0.0):.3f} ms" + " " * 47 + "║")
        print(f"║ Medium Priority CPU Cycles: {latest.get('medium_cpu_cycles', 0)}" + " " * 40 + "║")
        
        if len(metrics_history) > 1:
            print("╠" + "─" * 78 + "╣")
            print("║ Comparison (PI vs No PI):" + " " * 52 + "║")
            pi_metrics = [m for m in metrics_history if m.get('use_pi')]
            no_pi_metrics = [m for m in metrics_history if not m.get('use_pi')]
            
            if pi_metrics and no_pi_metrics:
                pi_avg = sum(m.get('avg_wait_time_ms', 0) for m in pi_metrics) / len(pi_metrics)
                no_pi_avg = sum(m.get('avg_wait_time_ms', 0) for m in no_pi_metrics) / len(no_pi_metrics)
                improvement = ((no_pi_avg - pi_avg) / no_pi_avg * 100) if no_pi_avg > 0 else 0
                print(f"║ Wait Time Improvement with PI: {improvement:.1f}%" + " " * 40 + "║")
    else:
        print("║ No demo results available yet. Run ./demo.sh or ./start.sh" + " " * 20 + "║")
    
    print("╠" + "═" * 78 + "╣")
    print("║ Active Processes:" + " " * 60 + "║")
    print("╠" + "─" * 78 + "╣")
    
    if process_metrics.get("processes"):
        for proc in process_metrics["processes"]:
            print(f"║ PID: {proc.get('pid', 'N/A')} | Name: {proc.get('name', 'N/A')} | " +
                  f"Priority: {proc.get('priority', 'N/A')} | Threads: {proc.get('threads', 'N/A')}" + 
                  " " * 20 + "║")
    else:
        print("║ No active priority_inversion processes" + " " * 42 + "║")
    
    print("╚" + "═" * 78 + "╝")
    print("\nPress Ctrl+C to exit")

def main():
    metrics_history = []
    
    # Load existing metrics
    if os.path.exists(METRICS_FILE):
        try:
            with open(METRICS_FILE, 'r') as f:
                data = json.load(f)
                if isinstance(data, list):
                    metrics_history = data
                elif isinstance(data, dict):
                    metrics_history = [data]
        except Exception:
            pass
    
    # Always try to load metrics from demo.log if it exists
    if os.path.exists(DEMO_LOG):
        try:
            with open(DEMO_LOG, 'r') as f:
                content = f.read()
                if content:
                    metrics = parse_demo_output(content)
                    # Add metrics if it has useful data (even if some values are 0)
                    if metrics.get("use_pi") is not None or metrics.get("high_lock_requests", 0) >= 0:
                        # Check if this metrics already exists (avoid duplicates)
                        is_duplicate = False
                        for existing in metrics_history:
                            if (existing.get("timestamp") == metrics.get("timestamp") and
                                existing.get("use_pi") == metrics.get("use_pi")):
                                is_duplicate = True
                                break
                        if not is_duplicate:
                            metrics_history.append(metrics)
                            # Keep only last 10 runs
                            metrics_history = metrics_history[-10:]
                            
                            # Save metrics
                            with open(METRICS_FILE, 'w') as f:
                                json.dump(metrics_history, f, indent=2)
        except Exception as e:
            pass
    
    # Monitor mode
    if len(sys.argv) > 1 and sys.argv[1] == "--monitor":
        print("Starting dashboard in monitor mode...")
        print("Press Ctrl+C to exit\n")
        
        try:
            while True:
                # Check for new demo output
                if os.path.exists(DEMO_LOG):
                    with open(DEMO_LOG, 'r') as f:
                        content = f.read()
                        if content:
                            metrics = parse_demo_output(content)
                            # Add metrics if it has useful data
                            if metrics.get("use_pi") is not None or metrics.get("high_lock_requests", 0) >= 0:
                                # Check if this metrics already exists (avoid duplicates)
                                is_duplicate = False
                                for existing in metrics_history:
                                    if (existing.get("timestamp") == metrics.get("timestamp") and
                                        existing.get("use_pi") == metrics.get("use_pi")):
                                        is_duplicate = True
                                        break
                                if not is_duplicate:
                                    metrics_history.append(metrics)
                                    # Keep only last 10 runs
                                    metrics_history = metrics_history[-10:]
                                    
                                    # Save metrics
                                    with open(METRICS_FILE, 'w') as f:
                                        json.dump(metrics_history, f, indent=2)
                
                # Get process metrics
                process_metrics = get_process_metrics()
                
                # Display dashboard
                display_dashboard(metrics_history, process_metrics)
                
                time.sleep(2)
        except KeyboardInterrupt:
            print("\n\nDashboard stopped.")
    else:
        # Single display mode
        process_metrics = get_process_metrics()
        display_dashboard(metrics_history, process_metrics)

if __name__ == "__main__":
    main()

