#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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

void handle_client(int);

int main() {
  int status;

  int s = socket(AF_INET, SOCK_STREAM, 0);
  check(&s, "Failed to create a socket");

  struct sockaddr_in server_addr = {
      AF_INET,
      htons(PORT),
      // SO_REUSEADDR,
      0,
  };

  status = bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  check(&status, "Failed to bind port to socket");

  status = listen(s, MAX_CONNECTIONS);
  check(&status, "Failed to listen for connections");

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd =
        accept(s, (struct sockaddr *)&client_addr, &client_addr_len);
    check(&client_fd, "Failed to accept connections");

    handle_client(client_fd);

    status = close(client_fd);
    check(&status, "Failed to close client socket");
  }

  status = close(s);
  check(&status, "Failed to close peer socket");

  return 0;
}

void handle_client(int client_fd) {
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
  if (status == 0) {
    printf("Client has closed the connection successfully\n");
    return;
  }
  check(&status, "Failed to receive data from client");
}
