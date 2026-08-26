#include "net.h"
#include "rules.h"
#include "stdio.h"

int main() {
  printf("Connecting to server...\n");
  int server_fd = init_connection();
  printf("Connected to server\n");

  printf("Awaiting color decision from server...\n");
  int player_color = get_color_from_server(server_fd);
  printf("Color received from server: %d\n", player_color);

  int board[BOARD_SIZE];
  for (int i = 0; i < BOARD_SIZE; i++)
    board[i] = 0;

  int status = send_board_to_server(server_fd, MOVE, 1, board);

  close_connection(server_fd);
}
