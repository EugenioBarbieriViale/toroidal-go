#ifndef NET_H
#define NET_H

#define BOARD_MSG_LEN (BOARD_SIZE + 3)
#define INIT_BUF_LEN 256

#define SERVER_IP "127.0.0.1"
const int PORT = 8080;

int talk_to_server(void);
int send_greetings(int);

int get_color(int);
void send_board_to_server(int, char *, int *);

#endif
