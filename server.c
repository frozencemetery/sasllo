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

#define DIE_IF(c)					  \
  if (c) {						  \
    fprintf(stderr, "Error in function %s at line %d!\n", \
	    __FUNCTION__, __LINE__);			  \
    goto fail;						  \
  }

char buf[HUGE]; /* I'm a terrible person */

static void get_connection(char *port, int *fd) {
  struct addrinfo hints, *serverdata = NULL;
  int conn = -1;
  int s = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int gai_ret = getaddrinfo(NULL, port, &hints, &serverdata);
  DIE_IF(gai_ret);

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
  DIE_IF(s == -1);

  DIE_IF(listen(s, 10) == -1);

  conn = accept(s, NULL, NULL);
  DIE_IF(conn == -1);

  *fd = conn;
  close(s);
  return;
 fail:
  if (serverdata) {
    freeaddrinfo(serverdata);
  }
  if (s != -1) {
    close(s);
  }
  if (conn != -1) {
    close(conn);
  }
  exit(1);
}

static void sasl_setup(char *port, sasl_conn_t **sconn) {
  char *iplocalport = NULL;
  int ret = asprintf(&iplocalport, "%s;%s", "0.0.0.0", port);
  DIE_IF(ret < 0 || !iplocalport);

  DIE_IF(sasl_server_init(NULL, SERVICE) != SASL_OK);

  DIE_IF(sasl_server_new(SERVICE, NULL, NULL, iplocalport, NULL, NULL, 0, 
			 sconn) != SASL_OK);

  return;
 fail:
  if (iplocalport) {
    free(iplocalport);
  }
  exit(1);
}

static void server_mech_negotiate(sasl_conn_t *sconn, int conn) {
  const char *mechs;
  unsigned mlen;
  DIE_IF(sasl_listmech(sconn, NULL, "", " ", "", &mechs, &mlen, NULL) !=
	 SASL_OK);

  ssize_t len;
  len = send(conn, mechs, mlen, 0);
  DIE_IF(len <= 0);

  len = recv(conn, buf, HUGE - 1, 0);
  DIE_IF(len <= 0);

  printf("mech: %s\n", buf);
  char *ind = strchr(buf, '\0') + 1;
  printf("data?: %d\n", ind ? 1 : 0);

  const char *serverout;
  unsigned serveroutlen;
  DIE_IF(sasl_server_start(sconn, buf, ind, len - (buf - ind),
			   &serverout, &serveroutlen) != SASL_OK);
  printf("%d, %s\n", serveroutlen, serverout);

  buf[0] = 's';
  memcpy(buf + 1, serverout, serveroutlen);
  len = send(conn, buf, serveroutlen + 1, 0);
  DIE_IF(len <= 0);

  return;
 fail:
  close(conn);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "%s <port>\n", argv[0]);
    return 1;
  }
  char *port = argv[1];

  sasl_conn_t *sconn;
  sasl_setup(port, &sconn);

  int conn;
  get_connection(port, &conn);

  server_mech_negotiate(sconn, conn);

  ssize_t len;
  while ((len = recv(conn, buf, HUGE - 1, 0)) > 0) {
    buf[len] = '\0';
    printf("%s\n", buf);
    len = send(conn, buf, len, 0);
    DIE_IF(len < 0);
  }

 fail:
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
