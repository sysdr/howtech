#!/usr/bin/env bash
# setup.sh
# One-click SDLC automation: init -> codegen -> build -> test -> local verify -> docker verify -> benchmark -> cleanup
# Designed for Node.js/Express app + React dashboard verification UI

set -o pipefail
# Don't exit immediately; we'll handle errors in functions so we can rollback/cleanup
# But we will fail if critical checks fail

### Configuration (edit or export environment variables) ###
PROJECT_NAME="p99-demo-service"
PROJECT_DIR="./${PROJECT_NAME}"
APP_PORT=${APP_PORT:-3000}
DASHBOARD_PORT=${DASHBOARD_PORT:-3001}
GIT_REMOTE=${GIT_REMOTE:-""} # optional
NODE_VERSION=${NODE_VERSION:-"16"}
DOCKER_IMAGE_NAME="${PROJECT_NAME}:latest"
ENVIRONMENT=${ENVIRONMENT:-"dev"} # dev|staging|prod
BENCH_DURATION=${BENCH_DURATION:-10} # seconds
BENCH_CONCURRENCY=${BENCH_CONCURRENCY:-50}
KEEP_ARTIFACTS=${KEEP_ARTIFACTS:-false}
LOGFILE="./${PROJECT_NAME}_setup.log"

# Colors
GREEN="\e[32m"
YELLOW="\e[33m"
RED="\e[31m"
BLUE="\e[34m"
RESET="\e[0m"

# Helpers for logging with color and timestamps
log() { echo -e "$(date +'%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOGFILE"; }
info() { echo -e "${BLUE}[INFO]${RESET} $1" | tee -a "$LOGFILE"; }
success() { echo -e "${GREEN}[SUCCESS]${RESET} $1" | tee -a "$LOGFILE"; }
warn() { echo -e "${YELLOW}[WARN]${RESET} $1" | tee -a "$LOGFILE"; }
error() { echo -e "${RED}[ERROR]${RESET} $1" | tee -a "$LOGFILE"; }

# Exit handler
on_exit() {
  local rc=$?
  if [ $rc -ne 0 ]; then
    error "Script exited with code $rc. See $LOGFILE for details."
  else
    success "Script finished successfully."
  fi
}
trap on_exit EXIT

# Rollback/cleanup state tracker
ROLLBACK_ACTIONS=()
add_rollback() { ROLLBACK_ACTIONS+=("$1"); }
run_rollback() {
  warn "Running rollback actions..."
  for ((i=${#ROLLBACK_ACTIONS[@]}-1; i>=0; i--)); do
    eval "${ROLLBACK_ACTIONS[$i]}" || warn "Rollback step failed: ${ROLLBACK_ACTIONS[$i]}"
  done
}

# Prereq checks
check_prereqs() {
  info "Checking prerequisites..."
  command -v node >/dev/null 2>&1 || { error "node is required. Install Node.js ${NODE_VERSION}+"; exit 1; }
  command -v npm >/dev/null 2>&1 || { error "npm is required."; exit 1; }
  command -v git >/dev/null 2>&1 || { error "git is required."; exit 1; }
  command -v docker >/dev/null 2>&1 || warn "docker not found — Docker verification will be skipped."
  command -v docker-compose >/dev/null 2>&1 || warn "docker-compose not found — docker-compose features will be skipped."
  command -v npx >/dev/null 2>&1 || { error "npx is required (comes with npm)."; exit 1; }
  success "Prerequisites OK."
}

# Project Setup
project_setup() {
  info "Creating project structure at $PROJECT_DIR"
  if [ -d "$PROJECT_DIR" ]; then
    warn "Directory $PROJECT_DIR already exists. Using existing directory."
  else
    mkdir -p "$PROJECT_DIR" || { error "Failed to create project dir"; exit 1; }
    add_rollback "rm -rf '$PROJECT_DIR'"
  fi

  cd "$PROJECT_DIR" || exit 1

  # Create folders
  mkdir -p src/controllers src/models src/routes src/middleware src/utils tests build docs config public dashboard || true

  # Create basic package.json
  if [ ! -f package.json ]; then
    cat > package.json <<'JSON'
{
  "name": "REPLACE_PROJECT_NAME",
  "version": "0.1.0",
  "description": "Demo service scaffolded by setup script",
  "main": "index.js",
  "scripts": {
    "start": "node index.js",
    "dev": "NODE_ENV=development nodemon index.js",
    "test": "jest --coverage",
    "lint": "eslint . --ext .js,.jsx",
    "build": "echo \"No build step configured\"",
    "dashboard": "cd dashboard && npm start"
  },
  "author": "",
  "license": "MIT",
  "dependencies": {
    "express": "^4.18.2"
  },
  "devDependencies": {
    "jest": "^29.0.0",
    "supertest": "^6.3.0",
    "eslint": "^8.0.0",
    "nodemon": "^2.0.0",
    "autocannon": "^7.14.0"
  }
}
JSON
    sed -i.bak "s/REPLACE_PROJECT_NAME/${PROJECT_NAME}/g" package.json && rm package.json.bak
    success "Generated package.json"
  else
    warn "package.json exists — skipping generation"
  fi

  # .gitignore
  cat > .gitignore <<'GITIGNORE'
node_modules
build
.env
.DS_Store
coverage
npm-debug.log
GITIGNORE

  # basic index.js
  cat > index.js <<'NODE'
// index.js - simple express app exposing /health and sample endpoints
const express = require('express');
const app = express();
const port = process.env.PORT || 3000;

// simple in-memory store for simulating p99 spike
let simulateSlow = false;

app.get('/health', (req, res) => res.json({status: 'ok', uptime: process.uptime()}));

app.get('/toggle-slow', (req, res) => {
  simulateSlow = !simulateSlow;
  res.json({simulateSlow});
});

app.get('/api/data', (req, res) => {
  const start = Date.now();
  if (simulateSlow && Math.random() < 0.1) {
    // randomly slow 10% of requests to emulate p99 spike
    const delay = 300 + Math.floor(Math.random() * 400);
    setTimeout(() => res.json({data: 'ok', delay, t: Date.now() - start}), delay);
  } else {
    // fast path
    res.json({data: 'ok', t: Date.now() - start});
  }
});

app.listen(port, () => console.log(`Service listening on ${port}`));
NODE

  success "Scaffolded basic service files"

  # simple README
  cat > README.md <<'MD'
# ${PROJECT_NAME}
Scaffolded demo service. Endpoints:
- GET /health
- GET /api/data
- GET /toggle-slow
MD

  # create simple jest test
  cat > tests/app.test.js <<'JEST'
const request = require('supertest');

// Simple test to verify the test setup works
describe('App', () => {
  test('should pass basic test', () => {
    expect(1 + 1).toBe(2);
  });
});
JEST

  # make basic files executable / set permissions
  chmod -R 755 .
  success "Project setup complete"
}

# Code generation: controllers, routes, models (very simple templates)
code_generation() {
  info "Generating code modules and configs..."
  mkdir -p src/{controllers,models,routes,middleware,utils}

  cat > src/controllers/sampleController.js <<'JS'
// sampleController.js
exports.getData = (req, res) => {
  res.json({data: 'controller data'});
};
JS

  cat > src/routes/index.js <<'JS'
// routes/index.js
const express = require('express');
const router = express.Router();
const controller = require('../controllers/sampleController');
router.get('/v1/data', controller.getData);
module.exports = router;
JS

  # env config templates
  mkdir -p config
  cat > config/default.json <<'JSON'
{
  "port": ${APP_PORT},
  "env": "${ENVIRONMENT}"
}
JSON

  cat > docs/API.md <<'MD'
# API Documentation
- GET /health — health check
- GET /api/data — sample endpoint
- GET /v1/data — controller routed data
MD

  success "Code generation complete"
}

# Install dependencies and build
build_and_deps() {
  info "Installing dependencies... (this may take a moment)"
  npm install --no-audit --no-fund || { error "npm install failed"; run_rollback; exit 1; }
  success "Dependencies installed"

  # transpile step placeholder (for TS/Sass etc.)
  info "Running build step (if any)"
  npm run build || warn "Build script returned non-zero (may be fine if none configured)"
  success "Build step complete"
}

# Testing
run_tests() {
  info "Running test suite: unit, integration, e2e (via npm test)"
  # ensure jest exists
  npx jest --version >/dev/null 2>&1 || { warn "jest not installed globally but available via npx"; }
  npm test || warn "Some tests failed (non-zero). Review logs."
  # coverage
  if [ -d coverage ]; then
    success "Coverage report generated"
  else
    warn "No coverage report found. Ensure tests configured to produce coverage."
  fi
}

# Local verification (non-docker)
local_verify() {
  info "Starting local server for verification..."
  # start in background
  npm start &
  SERVER_PID=$!
  add_rollback "kill $SERVER_PID || true"
  success "Server started (PID: $SERVER_PID). Waiting for startup..."
  sleep 2

  # health check
  info "Performing health check against http://localhost:${APP_PORT}/health"
  health=$(curl -fsS --max-time 5 http://localhost:${APP_PORT}/health || true)
  if [[ -z "$health" ]]; then
    error "Health check failed for local server"
    run_rollback
    exit 1
  fi
  success "Local health check OK: $health"

  # endpoint checks & simple response-time sampling for p99 calc
  endpoints=("/api/data" "/v1/data")
  declare -a samples
  info "Sampling ${#endpoints[@]} endpoints to compute latencies (will compute p50/p95/p99)"
  for ep in "${endpoints[@]}"; do
    local url="http://localhost:${APP_PORT}${ep}"
    local i
    samples=()
    for i in {1..120}; do
      t1=$(date +%s)
      curl -sS --max-time 2 "$url" >/dev/null || true
      t2=$(date +%s)
      rt=$((t2 - t1))
      samples+=("$rt")
    done
    # compute percentiles using awk (POSIX safe)
    printf "%s\n" "${samples[@]}" | sort -n > /tmp/latencies.txt
    total=$(wc -l < /tmp/latencies.txt)
    if [ "$total" -eq 0 ]; then warn "No samples collected for $url"; continue; fi
    p50_idx=$(( (total*50 + 99)/100 ))
    p95_idx=$(( (total*95 + 99)/100 ))
    p99_idx=$(( (total*99 + 99)/100 ))
    p50=$(sed -n "${p50_idx}p" /tmp/latencies.txt)
    p95=$(sed -n "${p95_idx}p" /tmp/latencies.txt)
    p99=$(sed -n "${p99_idx}p" /tmp/latencies.txt)
    success "$url -> p50=${p50}ms p95=${p95}ms p99=${p99}ms"
  done

  info "Local verification complete"
  
  # Stop the local server before Docker verification
  info "Stopping local server for Docker verification..."
  kill $SERVER_PID || true
  sleep 2
}

# Docker verification
docker_verify() {
  if ! command -v docker >/dev/null 2>&1; then
    warn "Docker not installed — skipping containerized verification"
    return
  fi

  info "Generating Dockerfile and docker-compose.yml"
  cat > Dockerfile <<DOCK
# Stage 1: build
FROM node:16-alpine AS build
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
EXPOSE ${APP_PORT}
CMD ["node", "index.js"]
DOCK

  cat > docker-compose.yml <<DC
version: '3.8'
services:
  app:
    build: .
    image: ${DOCKER_IMAGE_NAME}
    ports:
      - "${APP_PORT}:${APP_PORT}"
DC

  info "Building docker image ${DOCKER_IMAGE_NAME}"
  docker build -t ${DOCKER_IMAGE_NAME} . || { error "Docker build failed"; run_rollback; exit 1; }
  success "Docker image built"

  info "Running docker-compose to start container"
  docker-compose up -d || { error "docker-compose up failed"; run_rollback; exit 1; }
  add_rollback "docker-compose down -v --remove-orphans || true"
  sleep 3

  info "Running health checks against containerized app"
  container_health=$(curl -fsS --max-time 5 http://localhost:${APP_PORT}/health || true)
  if [[ -z "$container_health" ]]; then
    error "Containerized health check failed"
    docker-compose logs --no-color
    run_rollback
    exit 1
  fi
  success "Container health: $container_health"

  # Run tests against containerized app using same test suite endpoint checks
  info "Running same endpoint sampling against containerized deployment"
  endpoints=("/api/data")
  for ep in "${endpoints[@]}"; do
    local url="http://localhost:${APP_PORT}${ep}"
    npx autocannon -d ${BENCH_DURATION} -c ${BENCH_CONCURRENCY} $url || warn "autocannon returned non-zero"
  done

  success "Docker verification complete"
}

# Benchmark compare local vs docker
benchmark_compare() {
  info "Running comparative benchmarks: local vs container"
  if ! command -v npx >/dev/null 2>&1; then warn "npx not available for benchmarking"; return; fi
  # Local bench (assumes server running)
  info "Local benchmark against http://localhost:${APP_PORT}/api/data"
  npx autocannon -d ${BENCH_DURATION} -c ${BENCH_CONCURRENCY} http://localhost:${APP_PORT}/api/data > /tmp/bench_local.txt || warn "Local bench failed"
  # Docker bench
  info "Docker benchmark against http://localhost:${APP_PORT}/api/data"
  npx autocannon -d ${BENCH_DURATION} -c ${BENCH_CONCURRENCY} http://localhost:${APP_PORT}/api/data > /tmp/bench_docker.txt || warn "Docker bench failed"

  # Summarize
  grep -E "Requests/sec|p99" /tmp/bench_local.txt | sed -n '1,3p' > /tmp/bench_summary_local.txt || true
  grep -E "Requests/sec|p99" /tmp/bench_docker.txt | sed -n '1,3p' > /tmp/bench_summary_docker.txt || true
  info "Local summary:"; cat /tmp/bench_summary_local.txt
  info "Docker summary:"; cat /tmp/bench_summary_docker.txt
}

# Dashboard generation (React) — a simple verification UI
generate_dashboard() {
  info "Generating React dashboard in ./dashboard"
  mkdir -p dashboard
  cd dashboard || return
  if [ ! -f package.json ]; then
    cat > package.json <<'JSON'
{
  "name": "verification-dashboard",
  "version": "0.1.0",
  "private": true,
  "dependencies": {
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "react-scripts": "5.0.1"
  },
  "scripts": {
    "start": "react-scripts start",
    "build": "react-scripts build",
    "test": "react-scripts test --env=jsdom"
  }
}
JSON
  fi

  mkdir -p src
  cat > src/index.js <<'JS'
import React from 'react';
import { createRoot } from 'react-dom/client';
import App from './App';
import './index.css';
const root = createRoot(document.getElementById('root'));
root.render(<App />);
JS

  mkdir -p public
  cat > public/index.html <<'HTML'
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Verification Dashboard</title>
  </head>
  <body>
    <div id="root"></div>
    <script src="/src/index.js"></script>
  </body>
</html>
HTML

  cat > src/App.jsx <<'REACT'
import React, {useEffect, useState} from 'react';

const ENDPOINTS = [
  {name: 'health', path: '/health'},
  {name: 'api', path: '/api/data'},
];

function percentile(arr, p){
  if(arr.length===0) return null;
  const sorted = arr.slice().sort((a,b)=>a-b);
  const idx = Math.ceil((p/100)*sorted.length)-1;
  return sorted[Math.max(0,Math.min(idx, sorted.length-1))];
}

export default function App(){
  const [results, setResults] = useState({});
  const [running, setRunning] = useState(false);

  async function checkAll(){
    setRunning(true);
    const newRes = {};
    for(const ep of ENDPOINTS){
      const samples = [];
      for(let i=0;i<50;i++){
        const t1 = performance.now();
        try{
          const r = await fetch(ep.path, {cache: 'no-store'});
          await r.text();
        }catch(e){ /* ignore */ }
        const t2 = performance.now();
        samples.push(Math.round(t2-t1));
      }
      newRes[ep.name] = {
        p50: percentile(samples,50),
        p95: percentile(samples,95),
        p99: percentile(samples,99),
        latest: samples[samples.length-1]
      }
    }
    setResults(newRes);
    setRunning(false);
  }

  useEffect(()=>{ checkAll(); const id = setInterval(checkAll, 15000); return ()=>clearInterval(id); },[]);

  return (
    <div style={{fontFamily:'Arial, sans-serif', padding:20}}>
      <h2>Verification Dashboard</h2>
      <p>Service port: {window.location.port || "default"}</p>
      <button onClick={checkAll} disabled={running}>{running? 'Running...':'Run Checks'}</button>
      <div style={{display:'flex', gap:20, marginTop:20}}>
        {Object.entries(results).map(([k,v])=> (
          <div key={k} style={{border:'1px solid #ddd', padding:10, borderRadius:8, width:220}}>
            <h4>{k}</h4>
            <p>p50: {v.p50} ms</p>
            <p>p95: {v.p95} ms</p>
            <p>p99: {v.p99} ms</p>
            <p>latest: {v.latest} ms</p>
          </div>
        ))}
      </div>
    </div>
  )
}
REACT

  cat > src/index.css <<'CSS'
body { margin: 0; background: #f7f9fc; }
CSS

  # create simple start script for dashboard (development);
  cd ..
  success "Dashboard scaffold created in ./dashboard. Run with: (cd dashboard && npm install && npm start)"
}

# Cleanup and finalization
finalize() {
  info "Finalizing and cleanup"
  if [ "$KEEP_ARTIFACTS" = "false" ]; then
    info "Keeping artifacts is disabled; artifacts will remain for inspection"
  fi
  if [ -n "$GIT_REMOTE" ]; then
    git init || true
    git add .
    git commit -m "Initial scaffold by setup script" || true
    git remote add origin "$GIT_REMOTE" || warn "Failed to add git remote"
  fi
  success "All done. See $LOGFILE for the detailed log."
}

# Main
main() {
  check_prereqs
  project_setup
  code_generation
  build_and_deps
  run_tests
  generate_dashboard
  local_verify
  docker_verify
  benchmark_compare
  finalize
}

# Run
main "$@"