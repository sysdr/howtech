#!/usr/bin/env python3
"""
Dashboard server for File Descriptor Tracking Demo
Provides API endpoints and serves the dashboard
"""

import json
import os
import subprocess
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse, parse_qs

SCRIPT_DIR = Path(__file__).parent.parent
BUILD_DIR = SCRIPT_DIR / "build"
PID_FILE_LEAK = "/tmp/fd_demo_leak.pid"
PID_FILE_MONITOR = "/tmp/fd_demo_monitor.pid"


def get_pid_from_file(pid_file):
    """Read PID from file if it exists and process is still running"""
    if not os.path.exists(pid_file):
        return None
    try:
        with open(pid_file, 'r') as f:
            pid = int(f.read().strip())
        # Check if process is still running
        os.kill(pid, 0)
        return pid
    except (ValueError, OSError, ProcessLookupError):
        return None


def analyze_fds_for_process(pid):
    """Analyze file descriptors for a process and categorize them"""
    fd_count = 0
    regular_files = 0
    sockets = 0
    pipes = 0
    deleted_files = 0
    
    try:
        fd_dir = f"/proc/{pid}/fd"
        if not os.path.exists(fd_dir):
            return fd_count, regular_files, sockets, pipes, deleted_files
        
        for fd_name in os.listdir(fd_dir):
            if not fd_name.isdigit():
                continue
            
            fd_count += 1
            fd_path = os.path.join(fd_dir, fd_name)
            
            try:
                # Read the symlink target to determine FD type
                target = os.readlink(fd_path)
                
                if "socket:" in target:
                    sockets += 1
                elif "pipe:" in target:
                    pipes += 1
                elif "(deleted)" in target:
                    deleted_files += 1
                    regular_files += 1  # Deleted files count as regular files too
                else:
                    regular_files += 1
            except (OSError, PermissionError):
                # If we can't read the link, just count it as a regular file
                regular_files += 1
    except (OSError, PermissionError):
        pass
    
    return fd_count, regular_files, sockets, pipes, deleted_files


def count_fds_for_process(pid):
    """Count file descriptors for a process"""
    fd_count, _, _, _, _ = analyze_fds_for_process(pid)
    return fd_count


def get_process_info(pid):
    """Get process information with FD breakdown"""
    try:
        comm_file = f"/proc/{pid}/comm"
        if os.path.exists(comm_file):
            with open(comm_file, 'r') as f:
                name = f.read().strip()
            
            fd_count, regular_files, sockets, pipes, deleted_files = analyze_fds_for_process(pid)
            return {
                'pid': pid,
                'name': name,
                'fd_count': fd_count,
                'regular_files': regular_files,
                'sockets': sockets,
                'pipes': pipes,
                'deleted_files': deleted_files
            }
    except (OSError, PermissionError):
        pass
    return None


def scan_all_processes():
    """Scan all processes and get FD counts"""
    processes = []
    try:
        for entry in os.listdir("/proc"):
            if entry.isdigit():
                pid = int(entry)
                info = get_process_info(pid)
                if info and info['fd_count'] > 5:  # Only include processes with >5 FDs
                    processes.append(info)
    except (OSError, PermissionError):
        pass
    
    # Sort by FD count descending
    processes.sort(key=lambda x: x['fd_count'], reverse=True)
    return processes[:10]  # Top 10


def get_metrics():
    """Collect all metrics"""
    leak_pid = get_pid_from_file(PID_FILE_LEAK)
    monitor_pid = get_pid_from_file(PID_FILE_MONITOR)
    
    # Get top processes
    top_processes = scan_all_processes()
    
    # Calculate totals from all processes
    total_fds = sum(p['fd_count'] for p in top_processes)
    leaked_files = len([f for f in os.listdir("/tmp") if f.startswith("leaked_file_")]) if os.path.exists("/tmp") else 0
    
    # Calculate FD distribution from process data
    regular_files = sum(p.get('regular_files', 0) for p in top_processes)
    sockets = sum(p.get('sockets', 0) for p in top_processes)
    pipes = sum(p.get('pipes', 0) for p in top_processes)
    deleted_files = sum(p.get('deleted_files', 0) for p in top_processes)
    
    # Also scan all processes (not just top ones) for deleted files
    # since deleted files are important even in processes with few FDs
    try:
        for entry in os.listdir("/proc"):
            if entry.isdigit():
                pid = int(entry)
                # Quick scan just for deleted files
                try:
                    fd_dir = f"/proc/{pid}/fd"
                    if os.path.exists(fd_dir):
                        for fd_name in os.listdir(fd_dir):
                            if not fd_name.isdigit():
                                continue
                            fd_path = os.path.join(fd_dir, fd_name)
                            try:
                                target = os.readlink(fd_path)
                                if "(deleted)" in target:
                                    deleted_files += 1
                            except (OSError, PermissionError):
                                pass
                except (OSError, PermissionError):
                    pass
    except (OSError, PermissionError):
        pass
    
    return {
        'total_fds': total_fds,
        'process_count': len(top_processes),
        'leaked_files': leaked_files,
        'regular_files': regular_files,
        'sockets': sockets,
        'pipes': pipes,
        'deleted_files': deleted_files,
        'top_processes': top_processes,
        'demo': {
            'leak_pid': leak_pid,
            'monitor_pid': monitor_pid
        }
    }


class DashboardHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/api/metrics':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            metrics = get_metrics()
            self.wfile.write(json.dumps(metrics).encode())
        
        elif parsed_path.path == '/':
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.end_headers()
            index_path = Path(__file__).parent / "index.html"
            with open(index_path, 'rb') as f:
                self.wfile.write(f.read())
        
        elif parsed_path.path == '/favicon.ico':
            # Return 204 No Content for favicon to avoid 404 errors
            self.send_response(204)
            self.end_headers()
        
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        parsed_path = urlparse(self.path)
        
        if parsed_path.path.startswith('/api/start'):
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            # Check if demo is already running
            leak_pid = get_pid_from_file(PID_FILE_LEAK)
            monitor_pid = get_pid_from_file(PID_FILE_MONITOR)
            
            if leak_pid or monitor_pid:
                response = {
                    'status': 'already_running',
                    'message': 'Demo is already running',
                    'leak_pid': leak_pid,
                    'monitor_pid': monitor_pid
                }
            else:
                # Start the demo
                startup_script = SCRIPT_DIR / "startup.sh"
                if startup_script.exists():
                    try:
                        # Run startup script in background
                        process = subprocess.Popen(
                            ['bash', str(startup_script)],
                            cwd=str(SCRIPT_DIR),
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL
                        )
                        
                        # Wait for processes to start (check up to 5 times with delays)
                        leak_pid = None
                        monitor_pid = None
                        max_attempts = 5
                        for attempt in range(max_attempts):
                            time.sleep(1)
                            leak_pid = get_pid_from_file(PID_FILE_LEAK)
                            monitor_pid = get_pid_from_file(PID_FILE_MONITOR)
                            if leak_pid and monitor_pid:
                                break
                        
                        if leak_pid and monitor_pid:
                            response = {
                                'status': 'started',
                                'leak_pid': leak_pid,
                                'monitor_pid': monitor_pid
                            }
                        else:
                            # Check if processes exist even without PID files
                            leak_processes = subprocess.run(['pgrep', '-f', 'build/fd_leak'], 
                                                   capture_output=True, text=True)
                            monitor_processes = subprocess.run(['pgrep', '-f', 'build/fd_monitor'], 
                                                      capture_output=True, text=True)
                            if leak_processes.returncode == 0 or monitor_processes.returncode == 0:
                                response = {
                                    'status': 'started',
                                    'leak_pid': leak_pid,
                                    'monitor_pid': monitor_pid,
                                    'message': 'Processes started (PID files may be delayed)'
                                }
                            else:
                                response = {
                                    'status': 'error',
                                    'message': 'Failed to start demo processes - check if binaries exist'
                                }
                    except Exception as e:
                        response = {'status': 'error', 'message': str(e)}
                else:
                    response = {'status': 'error', 'message': 'startup.sh not found'}
            
            self.wfile.write(json.dumps(response).encode())
        
        elif parsed_path.path.startswith('/api/stop'):
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            # Stop the demo - more robust cleanup
            stopped_pids = []
            errors = []
            
            # Stop processes from PID files
            leak_pid = get_pid_from_file(PID_FILE_LEAK)
            monitor_pid = get_pid_from_file(PID_FILE_MONITOR)
            
            for pid in [leak_pid, monitor_pid]:
                if pid:
                    try:
                        # Try graceful shutdown first
                        os.kill(pid, 15)  # SIGTERM
                        time.sleep(1)
                        
                        # Check if process is still running, force kill if needed
                        try:
                            os.kill(pid, 0)  # Check if process exists
                            os.kill(pid, 9)  # SIGKILL if still running
                        except ProcessLookupError:
                            pass  # Process already terminated
                        
                        stopped_pids.append(pid)
                    except ProcessLookupError:
                        pass  # Process already dead
                    except Exception as e:
                        errors.append(f"Error stopping PID {pid}: {str(e)}")
            
            # Also kill any orphaned processes matching the pattern
            try:
                # Kill any fd_leak processes
                subprocess.run(['pkill', '-f', 'build/fd_leak'], 
                             stdout=subprocess.DEVNULL, 
                             stderr=subprocess.DEVNULL)
                # Kill any fd_monitor processes
                subprocess.run(['pkill', '-f', 'build/fd_monitor'], 
                             stdout=subprocess.DEVNULL, 
                             stderr=subprocess.DEVNULL)
            except Exception:
                pass
            
            # Clean up PID files
            try:
                if os.path.exists(PID_FILE_LEAK):
                    os.remove(PID_FILE_LEAK)
                if os.path.exists(PID_FILE_MONITOR):
                    os.remove(PID_FILE_MONITOR)
            except Exception as e:
                errors.append(f"Error cleaning PID files: {str(e)}")
            
            if errors:
                response = {
                    'status': 'stopped_with_errors',
                    'stopped_pids': stopped_pids,
                    'errors': errors
                }
            else:
                response = {
                    'status': 'stopped',
                    'stopped_pids': stopped_pids
                }
            
            self.wfile.write(json.dumps(response).encode())
        
        else:
            self.send_response(404)
            self.end_headers()
    
    def log_message(self, format, *args):
        # Suppress default logging
        pass


def main():
    port = int(os.environ.get('PORT', 8080))
    server = HTTPServer(('0.0.0.0', port), DashboardHandler)
    print(f"Dashboard server running on http://localhost:{port}")
    print("Press Ctrl+C to stop")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.shutdown()


if __name__ == '__main__':
    main()

