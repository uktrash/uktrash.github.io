#include "page.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define HTML_CAP (256 * 1024)
#define RESP_CAP (HTML_CAP + 512)
#define READ_CAP 2048

static volatile sig_atomic_t g_running = 1;

static void on_sigint(int sig) {
  (void)sig;
  g_running = 0;
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

static bool parse_request_line(const char *buf, char *method, size_t mcap,
                               char *path, size_t pcap) {
  if (sscanf(buf, "%15s %255s", method, path) != 2) {
    return false;
  }
  method[mcap - 1] = '\0';
  path[pcap - 1] = '\0';
  return true;
}

static int handle_client(int client_fd, const char *ok_response, size_t ok_len) {
  char req[READ_CAP];
  ssize_t n = recv(client_fd, req, sizeof(req) - 1, 0);
  if (n <= 0) {
    return -1;
  }
  req[n] = '\0';

  char method[16];
  char path[256];
  if (!parse_request_line(req, method, sizeof(method), path, sizeof(path))) {
    return -1;
  }

  static const char not_found[] =
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Cache-Control: no-store\r\n"
      "Connection: close\r\n"
      "Content-Length: 10\r\n"
      "\r\n"
      "not found\n";

  static const char health_ok[] =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Cache-Control: no-store\r\n"
      "Connection: close\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "ok\n";

  if (strcmp(method, "GET") != 0) {
    return write_all(client_fd, not_found, sizeof(not_found) - 1);
  }

  if (strcmp(path, "/") == 0) {
    return write_all(client_fd, ok_response, ok_len);
  }

  if (strcmp(path, "/healthz") == 0) {
    return write_all(client_fd, health_ok, sizeof(health_ok) - 1);
  }

  return write_all(client_fd, not_found, sizeof(not_found) - 1);
}

static int serve(uint16_t port, const char *ok_response, size_t ok_len) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  int one = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    close(fd);
    return 1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(fd);
    return 1;
  }

  if (listen(fd, 512) < 0) {
    perror("listen");
    close(fd);
    return 1;
  }

  fprintf(stderr, "serving on http://127.0.0.1:%u\n", (unsigned)port);
  while (g_running) {
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      close(fd);
      return 1;
    }

    (void)setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    (void)handle_client(client_fd, ok_response, ok_len);
    close(client_fd);
  }

  close(fd);
  return 0;
}

static int export_html(const char *path, const char *html, size_t html_len) {
  FILE *f = fopen(path, "wb");
  if (f == NULL) {
    perror("fopen");
    return 1;
  }

  if (fwrite(html, 1, html_len, f) != html_len) {
    perror("fwrite");
    fclose(f);
    return 1;
  }

  if (fclose(f) != 0) {
    perror("fclose");
    return 1;
  }

  fprintf(stderr, "exported %zu bytes to %s\n", html_len, path);
  return 0;
}

int main(int argc, char **argv) {
  char html[HTML_CAP];
  size_t html_len = render_portfolio_html(html, sizeof(html));
  if (html_len == 0 || html_len >= sizeof(html)) {
    fprintf(stderr, "failed to render html\n");
    return 1;
  }

  char response[RESP_CAP];
  int header_len = snprintf(
      response, sizeof(response),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "Cache-Control: no-store\r\n"
      "Connection: close\r\n"
      "Server: c-portfolio/1\r\n"
      "Content-Length: %zu\r\n"
      "\r\n",
      html_len);

  if (header_len <= 0) {
    fprintf(stderr, "failed to build header\n");
    return 1;
  }

  size_t hlen = (size_t)header_len;
  if (hlen + html_len > sizeof(response)) {
    fprintf(stderr, "response buffer too small\n");
    return 1;
  }

  memcpy(response + hlen, html, html_len);
  size_t ok_len = hlen + html_len;

  signal(SIGINT, on_sigint);
  signal(SIGTERM, on_sigint);

  if (argc == 3 && strcmp(argv[1], "--export") == 0) {
    return export_html(argv[2], html, html_len);
  }

  uint16_t port = DEFAULT_PORT;
  if (argc >= 2 && strcmp(argv[1], "--port") == 0 && argc >= 3) {
    long p = strtol(argv[2], NULL, 10);
    if (p < 1 || p > 65535) {
      fprintf(stderr, "invalid port\n");
      return 1;
    }
    port = (uint16_t)p;
  }

  return serve(port, response, ok_len);
}
