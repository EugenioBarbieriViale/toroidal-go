#ifndef NET_H
#define NET_H

#define BOARD_MSG_LEN (BOARD_SIZE + 3)
#define INIT_BUF_LEN 256

#define SERVER_IP "127.0.0.1"
#define PORT ((int)8080)

typedef enum {
  MOVE,
  PASS,
  RESIGN,
} MoveType;

int init_connection(void);
int close_connection(int);
int get_color_from_server(int);

int send_board_to_server(int, MoveType, int, int *);

#endif
