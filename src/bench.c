#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PORT 18080
#define DEFAULT_REQUESTS 5000U
#define DEFAULT_WARMUP 200U
#define READ_CAP 4096
#define PATH_CAP 256
#define REQUEST_CAP 512

struct stats {
  uint64_t min_ns;
  uint64_t p50_ns;
  uint64_t p95_ns;
  uint64_t p99_ns;
  uint64_t max_ns;
  double mean_ns;
  double throughput_rps;
};

static uint64_t now_ns(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    perror("clock_gettime");
    exit(1);
  }
  return ((uint64_t)ts.tv_sec * UINT64_C(1000000000)) + (uint64_t)ts.tv_nsec;
}

static int cmp_u64(const void *lhs, const void *rhs) {
  const uint64_t a = *(const uint64_t *)lhs;
  const uint64_t b = *(const uint64_t *)rhs;
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}

static double ns_to_us(uint64_t ns) { return (double)ns / 1000.0; }

static double ns_to_ms(uint64_t ns) { return (double)ns / 1000000.0; }

static double ns_to_seconds(double ns) { return ns / 1000000000.0; }

static double scaled_hour_seconds(double seconds) { return seconds * 3600.0; }

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s [--port N] [--path /route] [--requests N] [--warmup N]\n",
          argv0);
}

static bool parse_u16(const char *text, uint16_t *out) {
  char *end = NULL;
  errno = 0;
  unsigned long value = strtoul(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value == 0 || value > 65535UL) {
    return false;
  }
  *out = (uint16_t)value;
  return true;
}

static bool parse_u32(const char *text, uint32_t *out) {
  char *end = NULL;
  errno = 0;
  unsigned long value = strtoul(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value > 100000000UL) {
    return false;
  }
  *out = (uint32_t)value;
  return true;
}

static int write_all(int fd, const char *buf, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, buf + sent, len - sent, 0);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (n == 0) {
      return -1;
    }
    sent += (size_t)n;
  }
  return 0;
}

static int open_connection(uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return -1;
  }

  int one = 1;
  (void)setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
    close(fd);
    return -1;
  }

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    close(fd);
    return -1;
  }

  return fd;
}

static int do_request(uint16_t port, const char *request, size_t request_len,
                      size_t *response_bytes) {
  int fd = open_connection(port);
  if (fd < 0) {
    return -1;
  }

  if (write_all(fd, request, request_len) != 0) {
    close(fd);
    return -1;
  }

  char buf[READ_CAP];
  size_t total = 0;
  for (;;) {
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      close(fd);
      return -1;
    }
    if (n == 0) {
      break;
    }
    total += (size_t)n;
  }

  close(fd);
  *response_bytes = total;
  return 0;
}

static void compute_stats(struct stats *out, uint64_t *samples, uint32_t count,
                          uint64_t total_ns) {
  qsort(samples, (size_t)count, sizeof(samples[0]), cmp_u64);

  uint64_t sum_ns = 0;
  for (uint32_t i = 0; i < count; ++i) {
    sum_ns += samples[i];
  }

  const size_t last = (size_t)count - 1U;
  const size_t p50 = (size_t)((count * 50U) / 100U);
  const size_t p95 = (size_t)((count * 95U) / 100U);
  const size_t p99 = (size_t)((count * 99U) / 100U);

  out->min_ns = samples[0];
  out->p50_ns = samples[p50 > last ? last : p50];
  out->p95_ns = samples[p95 > last ? last : p95];
  out->p99_ns = samples[p99 > last ? last : p99];
  out->max_ns = samples[last];
  out->mean_ns = (double)sum_ns / (double)count;
  out->throughput_rps = (double)count / ns_to_seconds((double)total_ns);
}

int main(int argc, char **argv) {
  uint16_t port = DEFAULT_PORT;
  uint32_t requests = DEFAULT_REQUESTS;
  uint32_t warmup = DEFAULT_WARMUP;
  char path[PATH_CAP];
  memcpy(path, "/", 2U);

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      if (!parse_u16(argv[++i], &port)) {
        usage(argv[0]);
        return 1;
      }
      continue;
    }
    if (strcmp(argv[i], "--requests") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &requests) || requests == 0U) {
        usage(argv[0]);
        return 1;
      }
      continue;
    }
    if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &warmup)) {
        usage(argv[0]);
        return 1;
      }
      continue;
    }
    if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
      const char *arg = argv[++i];
      size_t len = strlen(arg);
      if (len == 0 || len >= sizeof(path) || arg[0] != '/') {
        usage(argv[0]);
        return 1;
      }
      memcpy(path, arg, len + 1U);
      continue;
    }
    usage(argv[0]);
    return 1;
  }

  uint64_t *samples = calloc((size_t)requests, sizeof(*samples));
  if (samples == NULL) {
    perror("calloc");
    return 1;
  }

  char request[REQUEST_CAP];
  int request_len = snprintf(request, sizeof(request),
                             "GET %s HTTP/1.1\r\n"
                             "Host: 127.0.0.1\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             path);
  if (request_len <= 0 || (size_t)request_len >= sizeof(request)) {
    fprintf(stderr, "request buffer too small\n");
    free(samples);
    return 1;
  }

  size_t response_bytes = 0;
  for (uint32_t i = 0; i < warmup; ++i) {
    if (do_request(port, request, (size_t)request_len, &response_bytes) != 0) {
      perror("warmup request");
      free(samples);
      return 1;
    }
  }

  const uint64_t bench_start = now_ns();
  for (uint32_t i = 0; i < requests; ++i) {
    const uint64_t start = now_ns();
    if (do_request(port, request, (size_t)request_len, &response_bytes) != 0) {
      perror("request");
      free(samples);
      return 1;
    }
    samples[i] = now_ns() - start;
  }
  const uint64_t bench_total_ns = now_ns() - bench_start;

  struct stats stats;
  compute_stats(&stats, samples, requests, bench_total_ns);

  printf("route: %s\n", path);
  printf("requests: %" PRIu32 "\n", requests);
  printf("warmup: %" PRIu32 "\n", warmup);
  printf("response_bytes: %zu\n", response_bytes);
  printf("throughput_rps: %.2f\n", stats.throughput_rps);
  printf("latency_min_us: %.2f\n", ns_to_us(stats.min_ns));
  printf("latency_p50_us: %.2f\n", ns_to_us(stats.p50_ns));
  printf("latency_p95_us: %.2f\n", ns_to_us(stats.p95_ns));
  printf("latency_p99_us: %.2f\n", ns_to_us(stats.p99_ns));
  printf("latency_max_us: %.2f\n", ns_to_us(stats.max_ns));
  printf("latency_avg_us: %.2f\n", stats.mean_ns / 1000.0);
  printf("\n");
  printf("translation:\n");
  printf("  median request time is %.3f ms, so one second can hold about %.0f back-to-back loads.\n",
         ns_to_ms(stats.p50_ns), 1000.0 / ns_to_ms(stats.p50_ns));
  printf("  99 out of 100 requests finished under %.3f ms.\n", ns_to_ms(stats.p99_ns));
  printf("  if one second were stretched to one hour, the median request would feel like %.2f seconds.\n",
         scaled_hour_seconds(ns_to_seconds((double)stats.p50_ns)));
  printf("  on that same one-hour scale, p99 lands at %.2f seconds.\n",
         scaled_hour_seconds(ns_to_seconds((double)stats.p99_ns)));

  free(samples);
  return 0;
}
