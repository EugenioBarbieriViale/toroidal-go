#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define N_LINES 19
#define BOARD_SIZE ((N_LINES) * (N_LINES))
#define INIT_BUF_LEN 256

#define SERVER_IP "127.0.0.1"
const int PORT = 8080;

static inline void check(int *status, const char *msg, int include_zero) {
  int condition = include_zero ? (*status <= 0) : (*status < 0);
  if (condition) {
    perror(msg);
    printf("errno: (%d)\n", errno);
    exit(EXIT_FAILURE);
  }
}

void send_board_to_server(int *, char *, int *);

typedef struct {
  char *name;
  char *value;
} Header;

typedef struct {
  char *status_line;
  Header *headers;
  int n_headers;
  int content_len;
  char *content;
} HttpResponse;

// HttpResponse parse_http(char *str) {
// void parse_http(char *_str) {
void parse_http(char *str) {
  while (1) {
    char *line_end = strstr(str, "\r\n");
    if (!line_end)
      exit(EXIT_FAILURE);

    int len = line_end - str;
    if (len == 0)
      break;

    char *line = malloc(len + 1);
    memcpy(line, str, len);

    line[len] = '\0';
    printf("%s\n", line);
    free(line);

    str = line_end + 2;
  }
}

int main() {
  int status;

  int s = socket(AF_INET, SOCK_STREAM, 0);
  check(&s, "Failed to create a socket", 0);

  struct sockaddr_in server_addr = {
      AF_INET,
      htons(PORT),
      0,
  };

  status = inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
  check(&status, "Invalid address", 1);

  status = connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  check(&status, "Failed to connect", 0);

  char board[BOARD_SIZE + 2];
  board[0] = 'N';
  board[1] = '-';
  for (int i = 2; i < BOARD_SIZE; i++)
    board[i] = '.';

  send_board_to_server(&s, board, &status);
  check(&status, "Failed to send message to socket", 0);

  char buf[INIT_BUF_LEN] = {' '};
  int n = recv(s, buf, INIT_BUF_LEN, 0);
  check(&n, "Failed to get reply from server", 1);

  printf("\n--- BEGIN SERVER REPLY ---\n");
  printf("%s\n", buf);
  printf("--- END SERVER REPLY ---\n\n");

  board[0] = '5';
  board[5] = 'O';
  send_board_to_server(&s, board, &status);
  check(&status, "Failed to send message to socket", 0);

  int new_n = recv(s, buf, INIT_BUF_LEN, 0);
  check(&new_n, "Failed to get reply from server", 1);

  printf("\n--- BEGIN SERVER REPLY ---\n");
  printf("%s\n", buf);
  printf("--- END SERVER REPLY ---\n\n");

  parse_http(buf);

  return 0;
}

void send_board_to_server(int *server_fd, char *board, int *status) {
  int tot_buf_size = 2 * INIT_BUF_LEN;
  char send_buf[tot_buf_size];

  int actual_len = snprintf(send_buf, tot_buf_size,
                            "POST / HTTP/1.1\r\n"
                            "Host: %s:%d\r\n"
                            "Content-type: text/plain\r\n"
                            "Content-length: %d\r\n"
                            "\r\n"
                            "%s\r\n",
                            SERVER_IP, PORT, BOARD_SIZE + 2, board

  );

  printf("Length of sent data (including header): %d\n", actual_len);
  printf("%s\n", board);

  *status = send(*server_fd, send_buf, actual_len, 0);
}
