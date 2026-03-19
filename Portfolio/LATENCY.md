# Latency Benchmarks

Measured on March 18, 2026 with the current native portfolio server.

## Test Setup

- Host: `Darwin arm64`
- Transport: local loopback on `127.0.0.1`
- Build flags: `-O3 -flto`
- Sample size: `5,000` measured requests per route
- Warmup: `200` requests per route
- Benchmark command: `make bench`

This is a best-case local benchmark. It measures the server process, the kernel networking path, and loopback I/O on this machine. It does not include public internet latency, CDN latency, TLS handshakes, or GitHub Pages behavior.

## Results

| Route | Response bytes | Throughput | p50 | p95 | p99 | Max | Avg |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `/` | 3143 | 18,548.54 req/s | 51 us | 69 us | 118 us | 978 us | 53.88 us |
| `/healthz` | 126 | 18,449.30 req/s | 50 us | 62 us | 96 us | 9,945 us | 54.17 us |

## Plain-English Translation

For the actual portfolio page at `/`:

- The median request finished in `0.051 ms`.
- That means one second can fit about `19,608` back-to-back page loads.
- `99 out of 100` requests finished in under `0.118 ms`.
- If one second were stretched into one hour, the median request would feel like about `0.18 seconds`, and the p99 request would feel like about `0.42 seconds`.

For the lightweight `/healthz` route:

- The median request finished in `0.050 ms`.
- That means one second can fit about `20,000` back-to-back health checks.
- `99 out of 100` requests finished in under `0.096 ms`.

## Reading The Outliers

The `max` column is useful, but it is not the number to lead with. Single outliers can be caused by scheduler noise, temporary system interrupts, or the benchmark process getting preempted. The more honest "real world on a quiet machine" number here is `p99`, not `max`.
