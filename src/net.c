#include "net.h"
#include "../http-server/httplib.h"
#include "rules.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// accept bool as condition, then message
static inline void check(int *status, const char *msg, int include_zero) {
  int condition = include_zero ? (*status <= 0) : (*status < 0);
  if (condition) {
    perror(msg);
    printf("errno: (%d)\n", errno);
    exit(EXIT_FAILURE);
  }
}

int talk_to_server() {
  int status;

  int s = socket(AF_INET, SOCK_STREAM, 0);
  check(&s, "Failed to create a socket", 0);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  status = inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
  check(&status, "Invalid address", 1);

  status = connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  check(&status, "Failed to connect", 0);

  status = send_greetings(s);
  check(&status, "Failed to send greetings", 0);

  const int color = get_color(s);
  printf("MY COLOR: %d\n", color);

  // if (color == BLACK) {
  //   char board[BOARD_SIZE + 1];
  //   for (int i = 0; i < BOARD_SIZE; i++)
  //     board[i] = EMPTY + '0';
  //   board[BOARD_SIZE] = '\0';
  //
  //   send_board_to_server(s, board, &status);
  //   check(&status, "Failed to send message to socket", 0);
  //
  //   char buf[INIT_BUF_LEN * 2];
  //   ssize_t n = recv(s, buf, INIT_BUF_LEN * 2, 0);
  //   if (n < 0) {
  //     perror("Failed to get board from server");
  //     exit(EXIT_FAILURE);
  //   }
  //   printf("%s\n", buf);
  // }
  return s;
}

int send_greetings(int server_fd) {
  char send_buf[INIT_BUF_LEN];

  int actual_len = snprintf(send_buf, INIT_BUF_LEN,
                            "GET / HTTP/1.1\r\n"
                            "Host: %s:%d\r\n"
                            "User-Agent: None\r\n"
                            "Content-type: text/plain\r\n"
                            "Content-length: 2\r\n"
                            "\r\n"
                            "*!\r\n",
                            SERVER_IP, PORT

  );

  int status = send(server_fd, send_buf, actual_len, 0);
  return status;
}

int get_color(int server_fd) {
  char buf[INIT_BUF_LEN] = {' '};
  int n = recv(server_fd, buf, INIT_BUF_LEN, 0);
  check(&n, "Failed to get color from server", 0);

  HttpResponse res = parse_http_response(buf);
  if (strlen(res.content) != 1 && res.content[0] != BLACK &&
      res.content[0] != WHITE) {
    printf("Not a valid color\n");
    exit(EXIT_FAILURE);
  }

  int c = res.content[0] - '0';
  free_response(&res);

  return c;
}

void send_board_to_server(int server_fd, char *board, int *status) {
  int tot_buf_size = 2 * INIT_BUF_LEN;
  char send_buf[tot_buf_size];

  int actual_len = snprintf(send_buf, tot_buf_size,
                            "POST / HTTP/1.1\r\n"
                            "Host: %s:%d\r\n"
                            "User-Agent: None\r\n"
                            "Content-type: text/plain\r\n"
                            "Content-length: %d\r\n"
                            "\r\n"
                            "B4-%s\r\n",
                            SERVER_IP, PORT, BOARD_MSG_LEN, board

  );

  printf("Length of sent data (including header): %d\n", actual_len);
  printf("%s\n", board);

  *status = send(server_fd, send_buf, actual_len, 0);
}
