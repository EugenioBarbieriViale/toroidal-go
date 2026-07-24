#include "httplib.h"
// #include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define N_LINES 19
#define BOARD_SIZE ((N_LINES) * (N_LINES))
#define INIT_BUF_LEN 256

const int MAX_CONNECTIONS = 10;
const int PORT = 8080;

static inline void check(int *status, const char *msg) {
  if (*status < 0) {
    perror(msg);
    printf("errno: (%d)\n", errno);
    exit(EXIT_FAILURE);
  }
}

int create_client(int);
void decide_colors(int, int);
int handle_client(int);

int main() {
  srand(time(NULL));
  int status;

  int s = socket(AF_INET, SOCK_STREAM, 0);
  check(&s, "Failed to create a socket");

  int yes = 1;
  status = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  check(&status, "Failed to set socket options");

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  status = bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  check(&status, "Failed to bind port to socket");

  status = listen(s, MAX_CONNECTIONS);
  check(&status, "Failed to listen for connections");

  printf("Waiting for connections...\n");

  int fd_a = create_client(s);
  int fd_b = create_client(s);

  fd_set readfs;

  int a_requested = 0, b_requested = 0;
  while (1) {

    FD_ZERO(&readfs);

    FD_SET(fd_a, &readfs);
    FD_SET(fd_b, &readfs);
    int max_fd = (fd_a > fd_b ? fd_a : fd_b);

    int ready = select(max_fd + 1, &readfs, NULL, NULL, NULL);
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      perror("Select error");
      break;
    }

    if (FD_ISSET(fd_a, &readfs)) {
      char buf[INIT_BUF_LEN];
      int n = recv(fd_a, buf, INIT_BUF_LEN, 0);
      HttpRequest req = parse_http_request(buf);
      if (strcmp(req.content, "*!") == 0 && n > 0) {
        a_requested = 1;
      }
    }

    if (FD_ISSET(fd_b, &readfs)) {
      char buf[INIT_BUF_LEN];
      int n = recv(fd_b, buf, INIT_BUF_LEN, 0);
      HttpRequest req = parse_http_request(buf);
      if (strcmp(req.content, "*!") == 0 && n > 0) {
        b_requested = 1;
      }
    }

    if (a_requested && b_requested) {
      decide_colors(fd_a, fd_b);
      a_requested = b_requested = 0;
    }
  }

  status = close(fd_a);
  check(&status, "Failed to close socket");

  status = close(fd_b);
  check(&status, "Failed to close socket");

  status = close(s);
  check(&status, "Failed to close peer socket");

  return 0;
}

int create_client(int s) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  int client_fd = accept(s, (struct sockaddr *)&client_addr, &client_addr_len);
  check(&client_fd, "Failed to accept connection from client");

  return client_fd;
}

void decide_colors(int fd_a, int fd_b) {
  int r = rand() % 2;
  char color_a = (r) ? 'O' : 'X';
  char color_b = (r) ? 'X' : 'O';

  char buf_a[INIT_BUF_LEN];
  int len_a = snprintf(buf_a, INIT_BUF_LEN,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: my-server\r\n"
                       "Content-type: text/plain\r\n"
                       "Content-length: 1\r\n"
                       "\r\n"
                       "%c\r\n",
                       color_a);

  char buf_b[INIT_BUF_LEN];
  int len_b = snprintf(buf_b, INIT_BUF_LEN,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: my-server\r\n"
                       "Content-type: text/plain\r\n"
                       "Content-length: 1\r\n"
                       "\r\n"
                       "%c\r\n",
                       color_b);

  int status = send(fd_a, buf_a, len_a, 0);
  check(&status, "Failed to send color to player A");

  status = send(fd_b, buf_b, len_b, 0);
  check(&status, "Failed to send color to player B");
}

int handle_client(int client_fd) {
  char buf[2 * INIT_BUF_LEN] = {' '};

  int status;
  while ((status = recv(client_fd, buf, 2 * INIT_BUF_LEN, 0)) > 0) {
    printf("\n--- BEGIN MSG ---\n");
    printf("%s\n", buf);
    printf("--- END MSG  ---\n\n");

    // check if buf is not empty
    if (1) {
      char reply[INIT_BUF_LEN] = "HTTP/1.1 200 OK\r\n"
                                 "Server: my-server\r\n"
                                 "Content-type: text/plain\r\n"
                                 "Content-length: 4\r\n"
                                 "\r\n"
                                 "OK\r\n";
      send(client_fd, reply, strlen(reply), 0);
    }
  }

  return status;
}
