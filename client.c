#include "common.h"

#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

static void get_server(char *host, char *port, int *fd) {
  struct addrinfo hints, *serverdata = NULL;
  int conn = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int gai_ret = getaddrinfo(host, port, &hints, &serverdata);
  DIE_IF(gai_ret);

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
  serverdata = NULL;
  DIE_IF(conn == -1);

  *fd = conn;
  return;
 fail:
  if (serverdata) {
    freeaddrinfo(serverdata);
  }
  if (conn != -1) {
    close(conn);
  }
  exit(1);
}

static void sasl_setup(char *host, char *port, sasl_conn_t **sconn) {
  int ret = -1;
  char *ipremoteport = NULL;

  ret = sasl_client_init(NULL);
  DIE_IF(ret != SASL_OK);

  ret = asprintf(&ipremoteport, "%s;%s", host, port);
  DIE_IF(ret < 0 || !ipremoteport);

  DIE_IF(sasl_client_new(SERVICE, host, NULL, ipremoteport, NULL, 0, sconn) !=
	 SASL_OK);

  return;
 fail:
  if (ipremoteport) {
    free(ipremoteport);
  }
  exit(1);
}

static void client_mech_negotiate(sasl_conn_t *sconn, int conn) {
  ssize_t len = recv(conn, buf, HUGE - 1, 0);
  DIE_IF(len <= 0);
  buf[len] = '\0';
  printf("mechs: %s\n", buf);

  const char *clientout = NULL;
  unsigned clientoutlen = 0;
  sasl_interact_t *prompt_need = NULL;
  const char *mech = NULL;
  DIE_IF(sasl_client_start(sconn, buf, &prompt_need,
			   &clientout, &clientoutlen, &mech) != SASL_OK);
  if (prompt_need) {
    printf("%s", "prompt_need!  Continuing anyway though.\n");
  }
  printf("chosen mech: %s\n", mech);

  strcpy(buf, mech);
  char *ind = buf + strlen(mech) + 1;
  memcpy(ind, clientout, clientoutlen);
  ind[clientoutlen] = '\0';

  len = send(conn, buf, (strlen(mech) + 1) + (clientoutlen + 1), 0);
  DIE_IF(len <= 0);

  len = recv(conn, buf, HUGE - 1, 0);
  DIE_IF(len <= 0);
  buf[len] = '\0';
  printf("dat: %d\n", len > 1);

  return;
 fail:
  close(conn);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "%s <host> <port>\n", argv[0]);
    return 1;
  }
  char *host = argv[1];
  char *port = argv[2];

  sasl_conn_t *sconn = NULL;
  sasl_setup(host, port, &sconn);

  int conn = -1;
  get_server(host, port, &conn);
  
  client_mech_negotiate(sconn, conn);

  ssize_t len;
  while ((len = fread(buf, 1, HUGE - 1, stdin)) > 0) {
    buf[(buf[len - 1] == '\n') ? len - 1 : len] = 0;
    len = send(conn, buf, len, 0);
    DIE_IF(len <= 0);
    len = recv(conn, buf, HUGE, 0);
    DIE_IF(len <= 0);
    printf("%s\n", buf);
  }

 fail:
  close(conn);

  printf("%s", "aaand we're good.\n");
  return 0;
}
