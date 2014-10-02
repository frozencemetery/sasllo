#define _GNU_SOURCE

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sasl/sasl.h>

#include <sys/socket.h>
#include <sys/types.h>

#define SERVICE "sasllo"
#define HUGE 4096

#define SASL_CHECK(c)                                      \
  do {                                                     \
    int __v = (c);                                         \
    if (__v != SASL_OK) {                                  \
      fprintf(stderr, "%s: %d\n", __FUNCTION__, __LINE__); \
      exit(__v);					   \
    }                                                      \
  } while (0);

static void get_server(char *host, char *port, int *fd) {
  struct addrinfo hints, *serverdata;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int gai_ret = getaddrinfo(host, port, &hints, &serverdata);
  if (gai_ret) {
    fprintf(stderr, "%s\n", gai_strerror(gai_ret));
    exit(gai_ret);
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
    exit(1);
  }

  *fd = conn;
  return;
}

static void sasl_setup(char *host, char *port, sasl_conn_t **sasl) {
  int ret;

  ret = sasl_client_init(NULL);
  if (ret != SASL_OK) {
    fprintf(stderr, "%s", "sasl_client_init error!\n");
    exit(ret);
  }

  char *ipremoteport;
  ret = asprintf(&ipremoteport, "%s;%s", host, port);
  if (ret < 0 || !ipremoteport) {
    fprintf(stderr, "no asprintf for you\n");
    exit(ret);
  }

  sasl_conn_t *sconn;
  SASL_CHECK(sasl_client_new(SERVICE,
			     host, NULL, ipremoteport, NULL, 0, &sconn));
  *sasl = sconn;
  return;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "%s <host> <port>\n", argv[0]);
    return 1;
  }
  char *host = argv[1];
  char *port = argv[2];

  sasl_conn_t *sconn;
  sasl_setup(host, port, &sconn);

  int conn;
  get_server(host, port, &conn);

  char *buf = calloc(1, HUGE);
  size_t len;

  /* get mechs */
  len = recv(conn, buf, HUGE - 1, 0);
  buf[len] = '\0';
  printf("mechs: %s\n", buf);

  const char *clientout = NULL;
  unsigned clientoutlen;
  sasl_interact_t *prompt_need = NULL;

  /* TODO(rharwood) I'm not convinced this meets spec */
  const char *mech;
  SASL_CHECK(sasl_client_start(
               sconn, buf, &prompt_need, &clientout, &clientoutlen, &mech));
  if (!prompt_need) {
    printf("!prompt_need\n");
  }
  printf("mech: %s\n", mech);

  strcpy(buf, mech);
  char *ind = buf + strlen(mech) + 1;
  memcpy(ind, clientout, clientoutlen);
  ind[clientoutlen] = '\0';

  send(conn, buf, (strlen(mech) + 1) + (clientoutlen + 1), 0);

  len = recv(conn, buf, HUGE - 1, 0);
  buf[len] = '\0';
  printf("dat: %s\n", buf + 1);

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
