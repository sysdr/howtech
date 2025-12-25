#!/usr/bin/env python3
"""
Dashboard for Kernel Tracepoints Demo
Displays real-time metrics from perf_monitor and target_program
"""

import os
import sys
import time
import json
import subprocess
import re
from pathlib import Path
from datetime import datetime

# ANSI colors
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[0;34m'
CYAN = '\033[0;36m'
NC = '\033[0m'  # No Color

SCRIPT_DIR = Path(__file__).parent.absolute()
LOGS_DIR = SCRIPT_DIR / "logs"
BUILD_DIR = SCRIPT_DIR / "build"

def clear_screen():
    """Clear the terminal screen"""
    os.system('clear' if os.name != 'nt' else 'cls')

def get_process_status():
    """Check if target_program and perf_monitor are running"""
    status = {
        'target_running': False,
        'monitor_running': False,
        'target_pid': None,
        'monitor_pid': None
    }
    
    try:
        result = subprocess.run(['pgrep', '-f', 'build/target_program'], 
                              capture_output=True, text=True)
        if result.returncode == 0 and result.stdout.strip():
            pids = result.stdout.strip().split('\n')
            status['target_running'] = True
            status['target_pid'] = pids[0] if pids else None
    except:
        pass
    
    try:
        result = subprocess.run(['pgrep', '-f', 'build/perf_monitor'], 
                              capture_output=True, text=True)
        if result.returncode == 0 and result.stdout.strip():
            pids = result.stdout.strip().split('\n')
            status['monitor_running'] = True
            status['monitor_pid'] = pids[0] if pids else None
    except:
        pass
    
    return status

def parse_perf_monitor_log():
    """Parse perf_monitor.log to extract metrics"""
    metrics = {
        'cpu_cycles': None,
        'instructions': None,
        'cache_misses': None,
        'ipc': None,
        'last_update': None
    }
    
    log_file = LOGS_DIR / "perf_monitor.log"
    if not log_file.exists():
        return metrics
    
    try:
        with open(log_file, 'r') as f:
            lines = f.readlines()
            # Look for the last data line (format: cycles instructions cache_misses ipc)
            for line in reversed(lines):
                # Skip header lines and error messages
                if re.match(r'^\s*\d+\s+\d+\s+\d+\s+[\d.]+\s*$', line.strip()):
                    parts = line.strip().split()
                    if len(parts) >= 4:
                        try:
                            metrics['cpu_cycles'] = int(parts[0].replace(',', ''))
                            metrics['instructions'] = int(parts[1].replace(',', ''))
                            metrics['cache_misses'] = int(parts[2].replace(',', ''))
                            metrics['ipc'] = float(parts[3])
                            metrics['last_update'] = datetime.now().strftime('%H:%M:%S')
                            break
                        except (ValueError, IndexError):
                            continue
    except Exception as e:
        pass
    
    return metrics

def parse_target_output_log():
    """Parse target_output.log to extract information"""
    info = {
        'iterations': 0,
        'last_iteration': None,
        'last_function_call': None
    }
    
    log_file = LOGS_DIR / "target_output.log"
    if not log_file.exists():
        return info
    
    try:
        with open(log_file, 'r') as f:
            lines = f.readlines()
            for line in lines:
                # Count iterations
                if 'Iteration' in line:
                    match = re.search(r'Iteration (\d+)', line)
                    if match:
                        info['iterations'] = int(match.group(1))
                        info['last_iteration'] = line.strip()
                # Get last function call
                if 'my_function called' in line:
                    info['last_function_call'] = line.strip()
    except Exception as e:
        pass
    
    return info

def format_number(num):
    """Format large numbers with commas"""
    if num is None:
        return "N/A"
    return f"{num:,}"

def display_dashboard():
    """Display the main dashboard"""
    clear_screen()
    
    print(f"{BLUE}{'='*70}{NC}")
    print(f"{BLUE}  Kernel Tracepoints Demo - Real-time Dashboard{NC}")
    print(f"{BLUE}{'='*70}{NC}")
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    # Process Status
    status = get_process_status()
    print(f"{CYAN}Process Status:{NC}")
    if status['target_running']:
        print(f"  {GREEN}✓{NC} Target Program: Running (PID: {status['target_pid']})")
    else:
        print(f"  {RED}✗{NC} Target Program: Not Running")
    
    if status['monitor_running']:
        print(f"  {GREEN}✓{NC} Perf Monitor: Running (PID: {status['monitor_pid']})")
    else:
        print(f"  {YELLOW}⚠{NC} Perf Monitor: Not Running (may need root/CAP_PERFMON)")
    print()
    
    # Performance Metrics
    metrics = parse_perf_monitor_log()
    print(f"{CYAN}Performance Metrics:{NC}")
    if metrics['cpu_cycles'] is not None:
        print(f"  CPU Cycles:      {format_number(metrics['cpu_cycles'])}")
        print(f"  Instructions:     {format_number(metrics['instructions'])}")
        print(f"  Cache Misses:     {format_number(metrics['cache_misses'])}")
        print(f"  IPC:              {metrics['ipc']:.2f}")
        if metrics['last_update']:
            print(f"  Last Update:      {metrics['last_update']}")
    else:
        print(f"  {YELLOW}No metrics available (perf_monitor may not be running){NC}")
    print()
    
    # Target Program Info
    target_info = parse_target_output_log()
    print(f"{CYAN}Target Program Info:{NC}")
    if target_info['iterations'] > 0:
        print(f"  Iterations:       {target_info['iterations']}")
        if target_info['last_iteration']:
            print(f"  Last Iteration:   {target_info['last_iteration']}")
        if target_info['last_function_call']:
            print(f"  Last Function:    {target_info['last_function_call']}")
    else:
        print(f"  {YELLOW}No activity logged${NC}")
    print()
    
    # System Info
    print(f"{CYAN}System Information:{NC}")
    try:
        # Check if tracefs is available
        if os.path.exists('/sys/kernel/debug/tracing'):
            print(f"  {GREEN}✓{NC} TraceFS: Available")
        else:
            print(f"  {YELLOW}⚠{NC} TraceFS: Not mounted (may need: sudo mount -t debugfs none /sys/kernel/debug)")
        
        # Check perf availability
        result = subprocess.run(['which', 'perf'], capture_output=True)
        if result.returncode == 0:
            print(f"  {GREEN}✓{NC} perf: Available")
        else:
            print(f"  {RED}✗{NC} perf: Not found")
    except:
        pass
    print()
    
    print(f"{BLUE}{'='*70}{NC}")
    print(f"Press Ctrl+C to exit | Auto-refresh every 2 seconds")
    print(f"{BLUE}{'='*70}{NC}")

def main():
    """Main dashboard loop"""
    try:
        while True:
            display_dashboard()
            time.sleep(2)
    except KeyboardInterrupt:
        print(f"\n{YELLOW}Dashboard stopped.${NC}")
        sys.exit(0)

if __name__ == "__main__":
    main()

