#!/usr/bin/env sh
set -eu

SERVER_BIN="./build/portfolio"
BENCH_BIN="./build/bench"
PORT="${PORT:-18080}"
REQUESTS="${REQUESTS:-5000}"
WARMUP="${WARMUP:-200}"

"$SERVER_BIN" --port "$PORT" >/tmp/portfolio-bench.out 2>/tmp/portfolio-bench.err &
PID=$!
trap 'kill "$PID" >/dev/null 2>&1 || true' EXIT

ATTEMPTS=0
until curl -s "http://127.0.0.1:$PORT/healthz" >/dev/null 2>&1; do
  ATTEMPTS=$((ATTEMPTS + 1))
  if [ "$ATTEMPTS" -gt 40 ]; then
    echo "server did not start" >&2
    exit 1
  fi
  sleep 0.1
done

echo "Benchmarking local loopback server on port $PORT"
echo
"$BENCH_BIN" --port "$PORT" --path "/" --requests "$REQUESTS" --warmup "$WARMUP"
echo
"$BENCH_BIN" --port "$PORT" --path "/healthz" --requests "$REQUESTS" --warmup "$WARMUP"
