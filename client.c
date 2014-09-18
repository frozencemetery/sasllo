#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sasl/sasl.h>

#include <sys/socket.h>
#include <sys/types.h>

#define HUGE 1024

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "%s <host> <port>\n", argv[0]);
    return 1;
  }
  char *host = argv[1];
  char *port = argv[2];

  int ret = sasl_client_init(NULL);
  if (ret != SASL_OK) {
    fprintf(stderr, "%s", "big mess come clean it up!\n");
    return ret;
  }

  struct addrinfo hints, *serverdata;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int gai_ret = getaddrinfo(host, port, &hints, &serverdata);
  if (gai_ret) {
    fprintf(stderr, "%s\n", gai_strerror(gai_ret));
    return 1;
  }

  int conn = -1;
  for (struct addrinfo *cur = serverdata; cur != NULL && conn == -1;
       cur = cur->ai_next) {
    conn = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
    if (conn == -1) {
      fprintf(stderr, "failed to socket\n");
    } else if (connect(conn, cur->ai_addr, cur->ai_addrlen) == -1) {
      close(conn);
      conn = -1;
      fprintf(stderr, "failed to connect\n");
    }
  }
  freeaddrinfo(serverdata);
  if (conn == -1) {
    fprintf(stderr, "unable to connect to any sockets\n");
    return 1;
  }

  char *buf = calloc(1, HUGE);
  size_t len;

  /* get mechs */
  len = recv(conn, buf, HUGE - 1, 0);
  buf[len] = '\0';
  printf("mechs: %s\n", buf);

  do {
    len = fread(buf, 1, HUGE - 1, stdin);
    if (len > 0 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
    } else {
      buf[len] = '\0';
    }
    len = send(conn, buf, len, 0);
  } while (len > 0);

  close(conn);

  printf("%s", "aaand we're good.\n");
  return 0;
}
