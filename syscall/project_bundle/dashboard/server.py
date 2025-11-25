"""
Lightweight dashboard server for syscall demo binaries.

Usage:
    python dashboard/server.py --port 8000
"""

from __future__ import annotations

import argparse
import json
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse
import time

PROJECT_ROOT = Path(__file__).resolve().parent.parent
MONITOR_DEMO_TEXT = (
    "Context Switches (per second)\n\n"
    "  Voluntary:     0\n\n"
    "  Involuntary:   0\n\n"
    "Total since start\n\n"
    "  Voluntary:     1\n\n"
    "  Involuntary:   0"
)


DASHBOARD_HTML = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>Syscall Insights Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    :root {
      color-scheme: dark;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background-color: #0f1115;
      color: #e2e5ec;
    }
    body {
      margin: 0 auto;
      padding: 24px;
      max-width: 900px;
    }
    h1 { margin-top: 0; }
    section {
      border: 1px solid #20222b;
      border-radius: 12px;
      padding: 20px;
      margin-bottom: 20px;
      background: #151821;
      box-shadow: 0 10px 25px rgba(0,0,0,0.35);
    }
    #demo_cards {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 16px;
    }
    .demo-card {
      border: 1px solid #2a2d3c;
      border-radius: 10px;
      padding: 16px;
      background: #0f1320;
    }
    .demo-card h3 {
      margin-top: 0;
      margin-bottom: 8px;
      font-size: 1rem;
      letter-spacing: .02em;
    }
    .monitor-actions {
      display: flex;
      gap: 12px;
      margin-top: 12px;
      flex-wrap: wrap;
    }
    button, input {
      font-size: 1rem;
      padding: 8px 14px;
      border-radius: 8px;
      border: 1px solid #2e303d;
      background: #1f2230;
      color: inherit;
    }
    button {
      cursor: pointer;
      background: linear-gradient(125deg, #3d65ef, #6d5dfa);
      border: none;
      transition: opacity 0.2s ease;
    }
    button:disabled {
      opacity: 0.6;
      cursor: wait;
    }
    pre {
      overflow-x: auto;
      padding: 12px;
      border-radius: 8px;
      background: #0b0d16;
      border: 1px solid #1d2030;
      min-height: 160px;
      white-space: pre-wrap;
      font-family: "JetBrains Mono", "Fira Code", "SFMono-Regular", monospace;
      font-size: 0.9rem;
      line-height: 1.4;
    }
    .status {
      font-size: .9rem;
      opacity: .8;
      margin-bottom: 8px;
    }
  </style>
</head>
<body>
  <h1>Syscall Insights Dashboard</h1>
  <p>Trigger the native binaries and inspect their real-time output from one place.</p>

  <section>
    <h2>Demo Snapshot</h2>
    <p class="status">Pre-captured data so you can see the format before running live commands.</p>
    <div id="demo_cards">
      <div class="demo-card">
        <h3>Latency Baseline</h3>
        <pre id="demo_latency"></pre>
      </div>
      <div class="demo-card">
        <h3>Assembly Output</h3>
        <pre id="demo_asm"></pre>
      </div>
      <div class="demo-card">
        <h3>Monitor Snapshot</h3>
        <pre id="demo_monitor"></pre>
      </div>
    </div>
  </section>

  <section>
    <h2>Syscall Latency Benchmark</h2>
    <p class="status">Runs <code>build/syscall_timing</code> and streams the tabular summary.</p>
    <button type="button" onclick="runTool('syscall_timing', '', this)">Run Benchmark</button>
    <pre id="syscall_timing_output">Awaiting benchmark run...</pre>
  </section>

  <section>
    <h2>Inline SYSCALL Demo</h2>
    <p class="status">Runs <code>build/syscall_asm</code> to show inline assembly output.</p>
    <button type="button" onclick="runTool('syscall_asm', '', this)">Run Assembly Demo</button>
    <pre id="syscall_asm_output">Awaiting assembly demo...</pre>
  </section>

  <section>
    <h2>Context Switch Monitor</h2>
    <p class="status">Streams <code>build/syscall_monitor &lt;pid&gt;</code> for ~3 seconds.</p>
    <form id="monitor-form">
      <label for="pid">Target PID:</label>
      <input id="pid" name="pid" type="text" placeholder="e.g. 12345" required />
      <div class="monitor-actions">
        <button type="submit" id="monitor-submit">Sample Monitor Output</button>
        <button type="button" id="monitor-demo-btn">Show Demo Data</button>
      </div>
    </form>
    <pre id="syscall_monitor_output">Provide a PID to monitor...</pre>
  </section>

  <script>
    const demoData = {
      syscall_timing: `+--------------------------------------------------------+
| SYSCALL INSTRUCTION LATENCY MEASUREMENT        |
+--------------------------------------------------------+
CPU Frequency: 2395.33 MHz
Measurement iterations: 1000000
getpid() via libc:           910 cycles   380 ns
getpid() via direct syscall: 885 cycles   369 ns
clock_gettime() via vDSO:    145 cycles    61 ns`,
      syscall_asm: `Hello from syscall instruction!
SYSCALL opcode: 0f 05`,
      syscall_monitor: `Context Switches (per second)
  Voluntary:     0
  Involuntary:   0
Total since start
  Voluntary:     1
  Involuntary:   0`,
    };

    document.getElementById("demo_latency").textContent = demoData.syscall_timing;
    document.getElementById("demo_asm").textContent = demoData.syscall_asm;
    document.getElementById("demo_monitor").textContent = demoData.syscall_monitor;

    const defaultOutputs = {
      syscall_timing: demoData.syscall_timing,
      syscall_asm: demoData.syscall_asm,
      syscall_monitor: demoData.syscall_monitor,
    };

    Object.entries(defaultOutputs).forEach(([tool, text]) => {
      const el = document.getElementById(`${tool}_output`);
      if (el) {
        el.textContent = text;
      }
    });

    async function runTool(tool, query = "", button = null) {
      const outputEl = document.getElementById(tool + "_output");
      if (button) {
        if (!button.dataset.label) {
          button.dataset.label = button.textContent;
        }
        button.disabled = true;
        button.textContent = "Running...";
      }
      outputEl.textContent = "Running " + tool + "...";
      try {
        const res = await fetch(`/api/${tool}${query}`);
        const body = await res.json();
        if (!res.ok || !body.success) {
          throw new Error(body.error || "Unknown error");
        }
        const primary = body.output && body.output.length ? body.output : "(no stdout)";
        const note = body.note ? `\n\n${body.note}` : "";
        const stderr = body.stderr ? `\n${body.stderr}` : "";
        outputEl.textContent = primary + note + stderr;
      } catch (err) {
        outputEl.textContent = "Error: " + err.message;
      } finally {
        if (button) {
          button.disabled = false;
          button.textContent = button.dataset.label;
        }
      }
    }

    const monitorForm = document.getElementById("monitor-form");
    const monitorSubmit = document.getElementById("monitor-submit");

    monitorForm.addEventListener("submit", (event) => {
      event.preventDefault();
      const pid = document.getElementById("pid").value.trim();
      if (!pid) {
        return;
      }
      runTool("syscall_monitor", `?pid=${encodeURIComponent(pid)}`, monitorSubmit);
    });

    document
      .getElementById("monitor-demo-btn")
      .addEventListener("click", (event) => {
        runTool("syscall_monitor", "?pid=demo", event.currentTarget);
      });
  </script>
</body>
</html>
"""


def run_binary(argv: list[str], timeout: int = 10) -> dict[str, str | int | bool]:
    """Execute a binary inside the project root and capture stdout/stderr."""
    try:
        result = subprocess.run(
            argv,
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired:
        return {"success": False, "error": "Command timed out"}
    except FileNotFoundError as exc:
        return {"success": False, "error": f"Executable not found: {exc}"}

    return {
        "success": result.returncode == 0,
        "returncode": result.returncode,
        "output": result.stdout.strip(),
        "stderr": result.stderr.strip(),
    }


def read_context_switches(pid: str) -> tuple[int, int] | None:
    """Return (voluntary, involuntary) counts for PID or None on failure."""
    status_path = Path("/proc") / pid / "status"
    try:
        with status_path.open("r", encoding="utf-8") as handle:
            voluntary = involuntary = None
            for line in handle:
                if line.startswith("voluntary_ctxt_switches:"):
                    voluntary = int(line.split(":", 1)[1].strip())
                elif line.startswith("nonvoluntary_ctxt_switches:"):
                    involuntary = int(line.split(":", 1)[1].strip())
                if voluntary is not None and involuntary is not None:
                    break
    except OSError:
        return None

    if voluntary is None or involuntary is None:
        return None
    return voluntary, involuntary


def sample_context_switches(pid: str, interval_s: float = 1.0) -> dict[str, object]:
    """Sample context switches over a small window and emit formatted text."""
    proc_dir = Path("/proc") / pid
    if not proc_dir.exists():
        return {
            "success": True,
            "output": MONITOR_DEMO_TEXT,
            "returncode": 0,
            "note": (
                f"Process {pid} is not running. Showing demo context-switch snapshot instead."
            ),
        }

    start = read_context_switches(pid)
    if start is None:
        return {
            "success": False,
            "error": (
                f"Unable to read /proc/{pid}/status. "
                "This PID may have exited or requires additional permissions."
            ),
        }

    time.sleep(interval_s)

    end = read_context_switches(pid)
    if end is None:
        return {"success": False, "error": f"Process {pid} ended before sampling completed"}

    vol_delta = max(0, end[0] - start[0])
    invol_delta = max(0, end[1] - start[1])

    lines = [
        "Context Switches (per second)",
        "",
        f"  Voluntary:     {vol_delta}",
        "",
        f"  Involuntary:   {invol_delta}",
        "",
        "Total since start",
        "",
        f"  Voluntary:     {end[0]}",
        "",
        f"  Involuntary:   {end[1]}",
    ]

    return {"success": True, "output": "\n".join(lines), "returncode": 0}


class DashboardHandler(BaseHTTPRequestHandler):
    server_version = "SyscallDashboard/1.0"

    def do_GET(self) -> None:  # noqa: N802 (BaseHTTPRequestHandler API)
        parsed = urlparse(self.path)
        if parsed.path == "/":
            self._send_response(200, "text/html", DASHBOARD_HTML)
            return

        if parsed.path == "/api/syscall_timing":
            payload = run_binary(["./build/syscall_timing"], timeout=15)
            self._send_json(payload)
            return

        if parsed.path == "/api/syscall_asm":
            payload = run_binary(["./build/syscall_asm"])
            self._send_json(payload)
            return

        if parsed.path == "/api/syscall_monitor":
            query = parse_qs(parsed.query)
            pid_values = query.get("pid")
            if not pid_values:
                self._send_json({"success": False, "error": "Missing pid query parameter"}, 400)
                return
            pid = pid_values[0]
            if pid.lower() == "demo":
                self._send_json(
                    {
                        "success": True,
                        "output": MONITOR_DEMO_TEXT,
                        "returncode": 0,
                        "note": "Showing demo context-switch snapshot.",
                    }
                )
                return
            if not pid.isdigit():
                self._send_json({"success": False, "error": "PID must be a numeric value"}, 400)
                return
            payload = sample_context_switches(pid)
            self._send_json(payload)
            return

        self._send_response(404, "text/plain", "Not Found")

    def log_message(self, format: str, *args) -> None:  # noqa: A003 (matches BaseHTTPRequestHandler)
        # Suppress default logging to keep console clean.
        return

    def _send_response(self, status: int, content_type: str, body: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body.encode("utf-8"))))
        self.end_headers()
        self.wfile.write(body.encode("utf-8"))

    def _send_json(self, payload: dict[str, object], status: int | None = None) -> None:
        status_code = status or (200 if payload.get("success") else 500)
        body = json.dumps(payload)
        self._send_response(status_code, "application/json", body)


def main() -> None:
    parser = argparse.ArgumentParser(description="Serve the syscall dashboard.")
    parser.add_argument("--host", default="127.0.0.1", help="Interface to bind (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8000, help="TCP port to listen on (default: 8000)")
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), DashboardHandler)
    print(f"Dashboard ready at http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down dashboard server...")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

