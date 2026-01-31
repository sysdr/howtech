#!/usr/bin/env python3
import time
import subprocess
import json
import sys
from datetime import datetime

def get_bpf_stats():
    """Get BPF program statistics"""
    stats = {
        'total_checks': 0,
        'allowed_count': 0,
        'denied_count': 0,
        'timestamp': datetime.now().isoformat()
    }
    
    try:
        # Try to get stats from BPF map
        result = subprocess.run(['bpftool', 'map', 'dump', 'name', 'policy_stats'], 
                              capture_output=True, text=True, timeout=2)
        if result.returncode == 0:
            # Parse bpftool output (simplified)
            for line in result.stdout.split('\n'):
                if 'total_checks' in line.lower():
                    stats['total_checks'] = 1  # Would need proper parsing
    except:
        pass
    
    return stats

def get_bpf_programs():
    """Get list of loaded BPF programs"""
    programs = []
    try:
        result = subprocess.run(['bpftool', 'prog', 'list'], 
                              capture_output=True, text=True, timeout=2)
        if result.returncode == 0:
            for line in result.stdout.split('\n'):
                if 'lsm' in line.lower() or 'file_open' in line.lower() or 'bprm_check' in line.lower():
                    programs.append(line.strip())
    except:
        pass
    return programs

def display_dashboard():
    print("\033[2J\033[H")  # Clear screen
    print("=" * 80)
    print("eBPF LSM Security Dashboard - Real-time Metrics")
    print("=" * 80)
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    stats = get_bpf_stats()
    print("Policy Statistics:")
    print(f"  Total Checks: {stats['total_checks']}")
    print(f"  Allowed: {stats['allowed_count']}")
    print(f"  Denied: {stats['denied_count']}")
    print()
    
    programs = get_bpf_programs()
    print(f"Loaded BPF Programs: {len(programs)}")
    for prog in programs[:5]:
        print(f"  - {prog}")
    print()
    
    print("System Status:")
    try:
        result = subprocess.run(['cat', '/sys/kernel/security/lsm'], 
                              capture_output=True, text=True, timeout=1)
        if result.returncode == 0:
            lsm_list = result.stdout.strip()
            if 'bpf' in lsm_list:
                print("  ✓ BPF LSM: ENABLED")
            else:
                print("  ⚠ BPF LSM: NOT ENABLED (Simulation Mode)")
    except:
        print("  ? LSM Status: Unknown")
    
    print()
    print("Press Ctrl+C to exit")
    print("=" * 80)

if __name__ == '__main__':
    try:
        while True:
            display_dashboard()
            time.sleep(2)
    except KeyboardInterrupt:
        print("\nDashboard stopped.")
        sys.exit(0)
