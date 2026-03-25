#include "page.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
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
#define MAX_CLIENTS 256

static volatile sig_atomic_t g_running = 1;

enum client_state {
  CLIENT_READING = 0,
  CLIENT_WRITING = 1,
};

struct client {
  int fd;
  char req[READ_CAP];
  size_t req_len;
  const char *resp;
  size_t resp_len;
  size_t sent;
  enum client_state state;
};

static const char NOT_FOUND[] =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Cache-Control: no-store\r\n"
    "Connection: close\r\n"
    "Content-Length: 10\r\n"
    "\r\n"
    "not found\n";

static const char HEALTH_OK[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Cache-Control: no-store\r\n"
    "Connection: close\r\n"
    "Content-Length: 3\r\n"
    "\r\n"
    "ok\n";

static const char BAD_REQUEST[] =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Cache-Control: no-store\r\n"
    "Connection: close\r\n"
    "Content-Length: 12\r\n"
    "\r\n"
    "bad request\n";

static void on_sigint(int sig) {
  (void)sig;
  g_running = 0;
}

static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    return -1;
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

static bool request_ready(const char *buf, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (buf[i] == '\n') {
      return true;
    }
  }
  return false;
}

static void reset_client(struct client *client, int fd) {
  client->fd = fd;
  client->req_len = 0;
  client->req[0] = '\0';
  client->resp = NULL;
  client->resp_len = 0;
  client->sent = 0;
  client->state = CLIENT_READING;
}

static void close_client(struct client *client) {
  if (client->fd >= 0) {
    close(client->fd);
  }
  reset_client(client, -1);
}

static void init_clients(struct client *clients) {
  for (size_t i = 0; i < MAX_CLIENTS; ++i) {
    reset_client(&clients[i], -1);
  }
}

static int reserve_client(struct client *clients) {
  for (size_t i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i].fd < 0) {
      return (int)i;
    }
  }
  return -1;
}

static void assign_response(struct client *client, const char *resp, size_t len) {
  client->resp = resp;
  client->resp_len = len;
  client->sent = 0;
  client->state = CLIENT_WRITING;
}

static void choose_response(struct client *client, const char *ok_response,
                            size_t ok_len) {
  char method[16];
  char path[256];

  client->req[client->req_len] = '\0';
  if (!parse_request_line(client->req, method, sizeof(method), path, sizeof(path))) {
    assign_response(client, BAD_REQUEST, sizeof(BAD_REQUEST) - 1U);
    return;
  }

  if (strcmp(method, "GET") != 0) {
    assign_response(client, NOT_FOUND, sizeof(NOT_FOUND) - 1U);
    return;
  }

  if (strcmp(path, "/") == 0) {
    assign_response(client, ok_response, ok_len);
    return;
  }

  if (strcmp(path, "/healthz") == 0) {
    assign_response(client, HEALTH_OK, sizeof(HEALTH_OK) - 1U);
    return;
  }

  assign_response(client, NOT_FOUND, sizeof(NOT_FOUND) - 1U);
}

static int read_client(struct client *client, const char *ok_response,
                       size_t ok_len) {
  for (;;) {
    if (client->req_len >= sizeof(client->req) - 1U) {
      assign_response(client, BAD_REQUEST, sizeof(BAD_REQUEST) - 1U);
      return 0;
    }

    ssize_t n = recv(client->fd, client->req + client->req_len,
                     sizeof(client->req) - 1U - client->req_len, 0);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0;
      }
      return -1;
    }

    if (n == 0) {
      return -1;
    }

    client->req_len += (size_t)n;
    client->req[client->req_len] = '\0';

    if (request_ready(client->req, client->req_len)) {
      choose_response(client, ok_response, ok_len);
      return 0;
    }
  }
}

static int flush_client(struct client *client) {
  while (client->sent < client->resp_len) {
    ssize_t n = send(client->fd, client->resp + client->sent,
                     client->resp_len - client->sent, 0);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0;
      }
      return -1;
    }
    if (n == 0) {
      return -1;
    }
    client->sent += (size_t)n;
  }
  return 1;
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

  if (set_nonblocking(fd) != 0) {
    perror("fcntl(O_NONBLOCK)");
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

  struct client clients[MAX_CLIENTS];
  init_clients(clients);

  fprintf(stderr, "serving on http://127.0.0.1:%u\n", (unsigned)port);
  while (g_running) {
    struct pollfd fds[MAX_CLIENTS + 1U];
    int slots[MAX_CLIENTS + 1U];
    nfds_t nfds = 1U;

    fds[0].fd = fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    slots[0] = -1;

    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
      if (clients[i].fd < 0) {
        continue;
      }
      fds[nfds].fd = clients[i].fd;
      fds[nfds].events = (short)(clients[i].state == CLIENT_READING ? POLLIN : POLLOUT);
      fds[nfds].revents = 0;
      slots[nfds] = (int)i;
      ++nfds;
    }

    int ready = poll(fds, nfds, 250);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("poll");
      for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        close_client(&clients[i]);
      }
      close(fd);
      return 1;
    }

    if (ready == 0) {
      continue;
    }

    if ((fds[0].revents & POLLIN) != 0) {
      for (;;) {
        int client_fd = accept(fd, NULL, NULL);
        if (client_fd < 0) {
          if (errno == EINTR) {
            continue;
          }
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }
          perror("accept");
          for (size_t i = 0; i < MAX_CLIENTS; ++i) {
            close_client(&clients[i]);
          }
          close(fd);
          return 1;
        }

        if (set_nonblocking(client_fd) != 0) {
          close(client_fd);
          continue;
        }
        (void)setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

        int slot = reserve_client(clients);
        if (slot < 0) {
          close(client_fd);
          continue;
        }
        reset_client(&clients[(size_t)slot], client_fd);
      }
    }

    for (nfds_t i = 1U; i < nfds; ++i) {
      struct client *client = &clients[(size_t)slots[i]];
      short revents = fds[i].revents;

      if (client->fd < 0) {
        continue;
      }

      if ((revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        close_client(client);
        continue;
      }

      if (client->state == CLIENT_READING && (revents & POLLIN) != 0) {
        if (read_client(client, ok_response, ok_len) != 0) {
          close_client(client);
          continue;
        }
      }

      if (client->fd < 0) {
        continue;
      }

      if (client->state == CLIENT_WRITING && (revents & POLLOUT) != 0) {
        int status = flush_client(client);
        if (status != 0) {
          close_client(client);
        }
      }
    }
  }

  for (size_t i = 0; i < MAX_CLIENTS; ++i) {
    close_client(&clients[i]);
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
