#!/usr/bin/env sh
set -eu

BIN="./build/portfolio"
PORT="${PORT:-18080}"

"$BIN" --port "$PORT" >/tmp/portfolio.out 2>/tmp/portfolio.err &
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

ROOT_CODE=$(curl -s -o /tmp/portfolio-root.html -w "%{http_code}" "http://127.0.0.1:$PORT/")
HEALTH_BODY=$(curl -s "http://127.0.0.1:$PORT/healthz")
MISS_CODE=$(curl -s -o /tmp/portfolio-miss.txt -w "%{http_code}" "http://127.0.0.1:$PORT/missing")

[ "$ROOT_CODE" = "200" ]
[ "$MISS_CODE" = "404" ]
[ "$HEALTH_BODY" = "ok" ]

echo "smoke: PASS"
