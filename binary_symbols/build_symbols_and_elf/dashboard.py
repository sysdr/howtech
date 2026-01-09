#!/usr/bin/env python3
"""
Binary Symbol Analysis Dashboard
Displays metrics and analysis from generated log files
"""

import os
import sys
import re
from pathlib import Path
from collections import defaultdict

def read_file_safe(filepath):
    """Safely read a file, return empty string if not found"""
    try:
        with open(filepath, 'r') as f:
            return f.read()
    except FileNotFoundError:
        return ""

def count_symbols_by_type(symbol_file):
    """Count symbols by type from nm output"""
    counts = defaultdict(int)
    content = read_file_safe(symbol_file)
    
    for line in content.split('\n'):
        if line.strip():
            parts = line.split()
            if len(parts) >= 2:
                symbol_type = parts[1]
                if symbol_type and symbol_type[0].isalpha():
                    counts[symbol_type[0]] += 1
    
    return counts

def analyze_disassembly(disasm_file):
    """Extract metrics from disassembly"""
    content = read_file_safe(disasm_file)
    
    # Count PLT entries
    plt_count = len(re.findall(r'@plt>', content))
    
    # Count function calls
    call_count = len(re.findall(r'\bcall\b', content, re.IGNORECASE))
    
    # Count instructions
    instruction_count = len([l for l in content.split('\n') if re.match(r'\s+[0-9a-f]+:', l)])
    
    return {
        'plt_entries': plt_count,
        'function_calls': call_count,
        'instructions': instruction_count
    }

def analyze_headers(headers_file):
    """Extract ELF header information"""
    content = read_file_safe(headers_file)
    
    sections = []
    for line in content.split('\n'):
        if re.match(r'^\s+\d+\s+\w+', line):
            parts = line.split()
            if len(parts) >= 2:
                sections.append(parts[1])
    
    return {
        'section_count': len(sections),
        'sections': sections[:10]  # First 10 sections
    }

def get_file_stats():
    """Get statistics about generated files"""
    stats = {}
    files_to_check = {
        'library_symbols': 'logs/library_symbols.txt',
        'detailed_symbols': 'logs/detailed_symbols.txt',
        'disassembly': 'logs/disassembly.txt',
        'headers': 'logs/headers.txt',
        'ld_debug': 'logs/ld_debug.txt',
        'strace': 'logs/strace.txt'
    }
    
    for name, path in files_to_check.items():
        if os.path.exists(path):
            size = os.path.getsize(path)
            with open(path, 'r') as f:
                lines = len(f.readlines())
            stats[name] = {'size': size, 'lines': lines, 'exists': True}
        else:
            stats[name] = {'exists': False}
    
    return stats

def check_binaries():
    """Check if binaries exist and are executable"""
    binaries = {
        'demo': 'build/demo',
        'library': 'build/libmylib.so',
        'monitor': 'build/monitor'
    }
    
    results = {}
    for name, path in binaries.items():
        exists = os.path.exists(path)
        executable = exists and os.access(path, os.X_OK)
        size = os.path.getsize(path) if exists else 0
        results[name] = {
            'exists': exists,
            'executable': executable,
            'size': size
        }
    
    return results

def print_dashboard():
    """Print the dashboard"""
    print("=" * 70)
    print("Binary Symbol Analysis Dashboard")
    print("=" * 70)
    print()
    
    # Binary status
    print("📦 Binary Status:")
    print("-" * 70)
    binaries = check_binaries()
    for name, info in binaries.items():
        status = "✅" if info['exists'] and info['executable'] else "❌"
        size_kb = info['size'] / 1024 if info['size'] > 0 else 0
        print(f"  {status} {name:10s} - {info['size']:8d} bytes ({size_kb:.1f} KB)")
    print()
    
    # Symbol statistics
    print("📊 Symbol Statistics:")
    print("-" * 70)
    symbol_counts = count_symbols_by_type('logs/library_symbols.txt')
    if symbol_counts:
        total = sum(symbol_counts.values())
        print(f"  Total symbols: {total}")
        print()
        
        type_descriptions = {
            'T': 'Global Code (text)',
            't': 'Local Code (text)',
            'D': 'Global Data',
            'd': 'Local Data',
            'B': 'Uninitialized Data (BSS)',
            'U': 'Undefined (external)',
            'W': 'Weak Symbol',
            'C': 'Common (uninitialized)'
        }
        
        for sym_type in sorted(symbol_counts.keys()):
            count = symbol_counts[sym_type]
            desc = type_descriptions.get(sym_type, 'Other')
            percentage = (count / total * 100) if total > 0 else 0
            print(f"  {sym_type:3s} {desc:30s} - {count:4d} ({percentage:5.1f}%)")
    else:
        print("  ⚠️  No symbol data available")
    print()
    
    # Disassembly metrics
    print("🔍 Disassembly Metrics:")
    print("-" * 70)
    disasm_metrics = analyze_disassembly('logs/disassembly.txt')
    if disasm_metrics['instructions'] > 0:
        print(f"  Instructions:      {disasm_metrics['instructions']:6d}")
        print(f"  Function calls:    {disasm_metrics['function_calls']:6d}")
        print(f"  PLT entries:       {disasm_metrics['plt_entries']:6d}")
    else:
        print("  ⚠️  No disassembly data available")
    print()
    
    # ELF Header info
    print("📋 ELF Structure:")
    print("-" * 70)
    header_info = analyze_headers('logs/headers.txt')
    if header_info['section_count'] > 0:
        print(f"  Sections: {header_info['section_count']}")
        if header_info['sections']:
            print(f"  Sample sections: {', '.join(header_info['sections'][:5])}")
    else:
        print("  ⚠️  No header data available")
    print()
    
    # File statistics
    print("📁 Generated Files:")
    print("-" * 70)
    file_stats = get_file_stats()
    for name, stats in file_stats.items():
        if stats.get('exists'):
            size_kb = stats['size'] / 1024
            print(f"  ✅ {name:20s} - {stats['lines']:5d} lines, {size_kb:6.1f} KB")
        else:
            print(f"  ❌ {name:20s} - Missing")
    print()
    
    # Demo status
    print("🎯 Demo Status:")
    print("-" * 70)
    if binaries['demo']['exists']:
        print("  ✅ Demo binary ready")
        print("  Run: ./build/demo")
    else:
        print("  ❌ Demo binary not found")
    
    if binaries['monitor']['exists']:
        print("  ✅ Monitor binary ready")
        print("  Run: ./build/monitor build/demo")
    else:
        print("  ❌ Monitor binary not found")
    print()
    
    print("=" * 70)

if __name__ == '__main__':
    # Change to script directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    print_dashboard()
    
    # Check if all metrics are available
    all_good = True
    binaries = check_binaries()
    file_stats = get_file_stats()
    
    if not all(b['exists'] for b in binaries.values()):
        print("\n⚠️  Warning: Some binaries are missing")
        all_good = False
    
    if not all(s.get('exists', False) for s in file_stats.values()):
        print("⚠️  Warning: Some log files are missing")
        all_good = False
    
    if all_good:
        print("\n✅ All metrics updated successfully!")
        sys.exit(0)
    else:
        print("\n❌ Some metrics are incomplete")
        sys.exit(1)

