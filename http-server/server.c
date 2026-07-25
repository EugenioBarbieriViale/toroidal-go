// board format:
// BN-board

#include "../src/rules.h"
#include "httplib.h"

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
#define BOARD_MSG_LEN (BOARD_SIZE + 3)

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

typedef struct {
  int fc;
  int color;

  int board[BOARD_SIZE];
  int all_neighbors[BOARD_SIZE][4];

  Stack reached;
  Stack chain;
} BoardState;

void init_bs(BoardState *bs) {
  bs->fc = -1;
  bs->color = UNDEF;

  for (int i = 0; i < BOARD_SIZE; i++) {
    bs->board[i] = EMPTY;
    get_neighbors(i, bs->all_neighbors[i]);
  }

  construct(&bs->reached);
  construct(&bs->chain);
}

int create_client(int);
void decide_colors(int, int, int *, int *);
int parse_board_msg(char *, BoardState *);
void handle_player(int, int, int *, BoardState *);

int main() {
  BoardState *bs = malloc(sizeof(BoardState));
  init_bs(bs);

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

  int color_a = UNDEF, color_b = UNDEF;

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
      handle_player(fd_a, color_a, &a_requested, bs);
    }

    if (FD_ISSET(fd_b, &readfs)) {
      handle_player(fd_b, color_b, &b_requested, bs);
    }

    if (a_requested && b_requested) {
      decide_colors(fd_a, fd_b, &color_a, &color_b);
      a_requested = b_requested = 0;
    }
  }

  status = close(fd_a);
  check(&status, "Failed to close socket");

  status = close(fd_b);
  check(&status, "Failed to close socket");

  status = close(s);
  check(&status, "Failed to close peer socket");

  free(bs);
  return 0;
}

int create_client(int s) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  int client_fd = accept(s, (struct sockaddr *)&client_addr, &client_addr_len);
  check(&client_fd, "Failed to accept connection from client");

  return client_fd;
}

void decide_colors(int fd_a, int fd_b, int *color_a, int *color_b) {
  int r = rand() % 2;
  *color_a = (r) ? BLACK : BLACK;
  *color_b = (r) ? WHITE : WHITE;

  char buf_a[INIT_BUF_LEN];
  int len_a = snprintf(buf_a, INIT_BUF_LEN,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: my-server\r\n"
                       "Content-type: text/plain\r\n"
                       "Content-length: 1\r\n"
                       "\r\n"
                       "%d\r\n",
                       *color_a);

  char buf_b[INIT_BUF_LEN];
  int len_b = snprintf(buf_b, INIT_BUF_LEN,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: my-server\r\n"
                       "Content-type: text/plain\r\n"
                       "Content-length: 1\r\n"
                       "\r\n"
                       "%d\r\n",
                       *color_b);

  int status = send(fd_a, buf_a, len_a, 0);
  check(&status, "Failed to send color to player A");

  status = send(fd_b, buf_b, len_b, 0);
  check(&status, "Failed to send color to player B");
}

static inline int is_digit(char c) { return (c >= '0' && c <= '9'); }
static inline int char_to_int(char c) { return (c - '0'); }

int parse_board_msg(char *board_msg, BoardState *bs) {
  int count = 0;
  if (board_msg[count++] != 'B') {
    printf(
        "Content not starting with correct identifier for board functions\n");
    return 1;
  }

  char *nu_end = memchr(board_msg, '-', count + 4);
  if (!nu_end) {
    printf("Invalid number, more than 3 digits\n");
    exit(EXIT_FAILURE);
  }

  int len = nu_end - board_msg;

  char *nu_str = malloc(len + 1);
  memcpy(nu_str, board_msg + count, len);
  nu_str[len] = '\0';

  int fc = atoi(nu_str);
  free(nu_str);

  if (fc < 0 || fc > BOARD_SIZE) {
    printf("Given coordinate is out of bounds: %d\n", fc);
    return 1;
  }

  if (board_msg[len] != '-') {
    printf("Board message is misformatted and does not contain a dash (-)\n");
    return 1;
  }

  board_msg = nu_end + 1;

  int given_board_size = strlen(board_msg);
  if (given_board_size != BOARD_SIZE) {
    printf(
        "The sent board is of length %d but was expected to have length %d\n",
        given_board_size, BOARD_SIZE);
    return 1;
  }

  bs->fc = fc;

  for (int i = 0; i < BOARD_SIZE; i++) {
    int maybe_stone = char_to_int(board_msg[i]);
    if (maybe_stone != EMPTY && maybe_stone != BLACK && maybe_stone != WHITE) {
      printf("Unidentified object at index %d: %d\n", i, maybe_stone);
      return 1;
    }

    bs->board[i] = maybe_stone;
  }

  return 0;
}

void handle_player(int fd, int color, int *request, BoardState *bs) {
  char buf[INIT_BUF_LEN * 2];

  int n = recv(fd, buf, INIT_BUF_LEN * 2, 0);
  printf("%s\n", buf);

  HttpRequest req = parse_http_request(buf);

  if (n == 0) {
    printf("Player has closed the connection successfully\n");
    free_request(&req);
    return;
  } else if (n > 0) {
    if (strcmp(req.content, "*!") == 0) {
      *request = 1;
      free_request(&req);
      return;
    }

    if (parse_board_msg(req.content, bs) == 1) {
      char err_buf[INIT_BUF_LEN];
      int len = snprintf(err_buf, INIT_BUF_LEN,
                         "HTTP/1.1 400 Bad Request\r\n"
                         "Server: my-server\r\n"
                         "Content-type: text/plain\r\n"
                         "Content-length: 1\r\n"
                         "\r\n"
                         "?\r\n");
      int status = send(fd, err_buf, len, 0);
      check(&status, "Failed to send error msg to player");
      free_request(&req);
      return;
    }

    // check rules, send YES / NO
    char move_msg_buf[INIT_BUF_LEN];
    int buf_len = INIT_BUF_LEN;
    int return_code = move(bs->fc, color, bs->board, bs->all_neighbors,
                           &bs->reached, &bs->chain, move_msg_buf, &buf_len);

    if (return_code != 0) {
      char err_buf[INIT_BUF_LEN];
      int len = snprintf(err_buf, INIT_BUF_LEN,
                         "HTTP/1.1 400 Bad Request\r\n"
                         "Server: my-server\r\n"
                         "Content-type: text/plain\r\n"
                         "Content-length: %d\r\n"
                         "\r\n"
                         "%s\r\n",
                         buf_len, move_msg_buf);
      int status = send(fd, err_buf, len, 0);
      check(&status, "Failed to send error msg to player");
    } else {
      printf("%s\n", move_msg_buf);

      char board_str[BOARD_SIZE + 1];
      for (int i = 0; i < BOARD_SIZE; i++) {
        board_str[i] = bs->board[i] + '0';
      }
      board_str[BOARD_SIZE] = '\0';

      char buf[INIT_BUF_LEN * 2];
      int len = snprintf(buf, INIT_BUF_LEN * 2,
                         "HTTP/1.1 200 OK\r\n"
                         "Server: my-server\r\n"
                         "Content-type: text/plain\r\n"
                         "Content-length: %d\r\n"
                         "\r\n"
                         "%s\r\n",
                         buf_len, board_str);

      int status = send(fd, buf, len, 0);
      check(&status, "Failed to send board to player");
    }
  }

  free_request(&req);
}
