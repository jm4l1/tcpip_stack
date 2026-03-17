#include "utils.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>

static struct termios *orig = NULL;

void apply_mask(char *prefix, char mask, char *str_prefix) {
  unsigned int binary_prefix = 0;
  inet_pton(AF_INET, prefix, &binary_prefix);
  binary_prefix = htonl(binary_prefix);
  unsigned int bit_mask = 0xFFFFFFFF << (32 - mask);
  binary_prefix &= bit_mask;
  binary_prefix = htonl(binary_prefix);
  inet_ntop(AF_INET, &binary_prefix, str_prefix, INET_ADDRSTRLEN);
  str_prefix[15] = '\0';
}

void layer2_fill_with_broadcast_mac(char *mac_array) {
  for (int idx = 0; idx < 6; idx++) {
    mac_array[idx] = 0xFF;
  }
};

void print_termios(struct termios *term) {
  if (term != NULL) {
    printf("{ iflag: %d ,oflag: %d,cflag: %d,lflag: %d,line: %c,cc: %c,ispeed: "
           "%d,ospeed: %d}\n",
           term->c_iflag,    /* input mode flags */
           term->c_oflag,    /* output mode flags */
           term->c_cflag,    /* control mode flags */
           term->c_lflag,    /* local mode flags */
           term->c_line,     /* line discipline */
           term->c_cc[NCCS], /* control characters */
           term->c_ispeed,   /* input speed */
           term->c_ospeed /* output speed */);
  }
}

void set_raw_mode() {
  if (orig == NULL) {
    orig = calloc(1, sizeof(struct termios));
  }
  struct termios raw;
  tcgetattr(STDIN_FILENO, orig);
  raw = *orig;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void restore_mode() {
  if (orig != NULL) {
    tcsetattr(STDIN_FILENO, TCSANOW, orig);
  }
}