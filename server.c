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

static void get_connection(char *port, int *fd) {
  struct addrinfo hints, *serverdata;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int gai_ret = getaddrinfo(NULL, port, &hints, &serverdata);
  if (gai_ret) {
    fprintf(stderr, "%s\n", gai_strerror(gai_ret));
    exit(gai_ret);
  }

  int s = -1;
  int reuseaddr = 1;
  for (struct addrinfo *cur = serverdata; s == -1 && cur != NULL;
       cur = cur->ai_next) {
    s = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
    if (s == -1) {
      fprintf(stderr, "failed to socket\n");
    } else if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                          &reuseaddr, sizeof(int)) == -1) {
      close(s);
      s = -1;
      fprintf(stderr, "setsockopt failure\n");
    } else if (bind(s, cur->ai_addr, cur->ai_addrlen) == -1) {
      fprintf(stderr, "failed to bind\n");
      close(s);
      s = -1;
    }
  }
  freeaddrinfo(serverdata);
  if (s == -1) {
    fprintf(stderr, "unable to acquire a socket\n");
    exit(1);
  }

  if (listen(s, 10) == -1) {
    fprintf(stderr, "failed to listen\n");
    close(s);
    exit(1);
  }

  int conn = accept(s, NULL, NULL);
  if (conn == -1) {
    fprintf(stderr, "failed to accept\n");
    exit(1);
  }

  *fd = conn;
  close(s);
  return;
}

static void sasl_setup(char *port, sasl_conn_t **sasl) {
  char *iplocalport;
  int ret = asprintf(&iplocalport, "%s;%s", "0.0.0.0", port);
  if (ret < 0 || !iplocalport) {
    fprintf(stderr, "no asprintf for you\n");
    exit(ret);
  }

  SASL_CHECK(sasl_server_init(NULL, SERVICE));

  sasl_conn_t *sconn;
  SASL_CHECK(sasl_server_new(SERVICE,
			     NULL, NULL, iplocalport, NULL, NULL, 0, &sconn));

  *sasl = sconn;
  return;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "%s <port>\n", argv[0]);
    return 1;
  }
  char *port = argv[1];

  sasl_conn_t *sconn;
  sasl_setup(port, &sconn);

  const char *mechs;
  unsigned mlen;
  SASL_CHECK(sasl_listmech(sconn, NULL, "", " ", "", &mechs, &mlen, NULL));

  int conn;
  get_connection(port, &conn);

  /* send mechs */
  send(conn, mechs, mlen, 0);

  char *buf = calloc(1, HUGE);
  int len;

  len = recv(conn, buf, HUGE - 1, 0);
  printf("mech: %s\n", buf);
  char *ind = strchr(buf, '\0') + 1;
  printf("data?: %d\n", ind ? 1 : 0);

  const char *serverout;
  unsigned serveroutlen;
  SASL_CHECK(sasl_server_start(sconn, buf, ind, len - (buf - ind),
                               &serverout, &serveroutlen));
  printf("%d, %s\n", serveroutlen, serverout);

  buf[0] = 's';
  memcpy(buf + 1, serverout, serveroutlen);
  len = send(conn, buf, serveroutlen + 1, 0);

  do {
    len = recv(conn, buf, HUGE - 1, 0);
    buf[(len > 0) ? len : 0] = '\0';
    printf("%s\n", buf);
    len = send(conn, buf, len, 0);
    if (len < 0) {
      fprintf(stderr, "failed to send!\n");
      return 1;
    }
  } while (len > 0);

  close(conn);

  /***/

  /* uchar *sdf = calloc(1, HUGE); */
  /* uchar *ptr = sdf + 4; */

  /* *ptr++ = 0x02; /\* DOFF *\/ */
  /* *ptr++ = 0x01; /\* SASL *\/ */
  /* *ptr++ = 0x00; */
  /* *ptr++ = 0x00; */

  /* *ptr++ = 0x00; /\* composite *\/ */
  /* *ptr++ = 0x53; /\* one *\/ */
  /* *ptr++ = 0x40; /\* mechs *\/ */

  /* *ptr++ = 0xd0; /\* list *\/ */
  /* ptr += 4; /\* skip list length *\/ */
  /* *ptr++ = 0x00; */
  /* *ptr++ = 0x00; */
  /* *ptr++ = 0x00; */
  /* *ptr++ = 0x01; */

  /* *ptr++ = 0xf0; */
  /* ptr += 4; /\* skip list length *\/ */
  /* *ptr++ = 0x00; */
  /* *ptr++ = 0x00; */
  /* *ptr++ = 0x00; */
  /* *ptr++ = (uchar) mlen; */

  /* *ptr++ = 0xb3; /\* string *\/ */
  /* /\* XXX for each string *\/ */
  /* ptr += 4; /\* skip string length *\/ */
  /* /\* XXX write first string *\/ */

  /* /\***\/ */

  /* /\* They've got XML shits for this; I should steal it for reasons *\/ */

  printf("%s", "aaand we're good.\n");
  return 0;
}
