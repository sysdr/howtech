#!/usr/bin/env python3
import gdb
import sys

class CoreAnalyzer:
    def __init__(self):
        self.results = []
    
    def log(self, msg):
        self.results.append(msg)
        print(msg)
    
    def analyze_threads(self):
        self.log("\n=== Thread Analysis ===")
        try:
            output = gdb.execute("info threads", to_string=True)
            self.log(output)
            
            # Get backtrace for each thread
            threads = gdb.execute("info threads", to_string=True).split('\n')
            for line in threads:
                if line.strip() and line[0].isdigit():
                    parts = line.split()
                    if len(parts) > 0:
                        thread_id = parts[0].replace('*', '').strip()
                        try:
                            gdb.execute(f"thread {thread_id}", to_string=True)
                            self.log(f"\n--- Thread {thread_id} Backtrace ---")
                            bt = gdb.execute("bt", to_string=True)
                            self.log(bt)
                        except:
                            pass
        except Exception as e:
            self.log(f"Error analyzing threads: {e}")
    
    def analyze_connection_pool(self):
        self.log("\n=== Connection Pool Analysis ===")
        try:
            # Try to access global_conn_pool
            pool = gdb.parse_and_eval("global_conn_pool")
            if pool:
                size = int(pool['size'])
                capacity = int(pool['capacity'])
                self.log(f"Pool size: {size}/{capacity}")
                
                # Count connections by state
                states = {0: 0, 1: 0, 2: 0, 3: 0}
                if size > 0 and size < 1000:  # Safety check
                    for i in range(min(size, 100)):  # Sample first 100
                        try:
                            conn = pool['connections'][i]
                            state = int(conn['state'])
                            states[state] = states.get(state, 0) + 1
                        except:
                            pass
                
                self.log(f"\nConnection states (sampled):")
                self.log(f"  CLOSED: {states.get(0, 0)}")
                self.log(f"  CONNECTING: {states.get(1, 0)}")
                self.log(f"  ESTABLISHED: {states.get(2, 0)}")
                self.log(f"  CLOSE_WAIT: {states.get(3, 0)}")
                
                if states.get(3, 0) > 10:
                    self.log(f"\n⚠️  HIGH CLOSE_WAIT count detected!")
                    self.log(f"   This indicates a connection leak!")
        except Exception as e:
            self.log(f"Could not analyze connection pool: {e}")
    
    def analyze_registers(self):
        self.log("\n=== Register State ===")
        try:
            regs = gdb.execute("info registers", to_string=True)
            self.log(regs)
        except Exception as e:
            self.log(f"Error reading registers: {e}")
    
    def analyze_signal(self):
        self.log("\n=== Signal Information ===")
        try:
            # Get signal info
            siginfo = gdb.execute("info program", to_string=True)
            self.log(siginfo)
        except Exception as e:
            self.log(f"Error reading signal info: {e}")
    
    def analyze_memory_map(self):
        self.log("\n=== Memory Map Summary ===")
        try:
            maps = gdb.execute("info proc mappings", to_string=True)
            # Parse and summarize
            lines = maps.split('\n')
            total_size = 0
            anon_count = 0
            file_count = 0
            
            for line in lines:
                if 'KB' in line or 'MB' in line or 'GB' in line:
                    if '[heap]' in line or '[stack]' in line or line.strip().endswith('0'):
                        anon_count += 1
                    else:
                        file_count += 1
            
            self.log(f"Anonymous mappings: {anon_count}")
            self.log(f"File-backed mappings: {file_count}")
            self.log(f"\nFirst 20 mappings:")
            self.log('\n'.join(lines[3:23]))
        except Exception as e:
            self.log(f"Error reading memory map: {e}")
    
    def run_analysis(self):
        self.log("=== Core Dump Analysis Report ===")
        self.log(f"Generated: {gdb.execute('show version', to_string=True).split(chr(10))[0]}")
        
        self.analyze_signal()
        self.analyze_registers()
        self.analyze_threads()
        self.analyze_connection_pool()
        self.analyze_memory_map()
        
        self.log("\n=== Analysis Complete ===")
        
        # Save to file
        with open('/tmp/core_analysis.txt', 'w') as f:
            f.write('\n'.join(self.results))
        
        print("\n📝 Full analysis saved to /tmp/core_analysis.txt")

# Run the analysis
analyzer = CoreAnalyzer()
analyzer.run_analysis()

# Quit GDB
gdb.execute("quit")
