#include "common.h"

#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

void encode_send(sasl_conn_t *sconn, int conn,
		 const char *data, unsigned datalen) {
  const char *output = NULL;
  unsigned outputlen;
  int ret = sasl_encode(sconn, data, datalen, &output, &outputlen);
  if (ret == SASL_TOOWEAK) {
    /* Due to being designed by crypto people, the library will not
     * allow for use of sasl_encode / sasl_decode when the mechanism
     * is ANONYMOUS.  In my opinion, they should just make output and
     * outputlen copies of input and inputlen, so this creates that
     * behavior. */
    output = data;
    outputlen = datalen;
  } else DIE_IF(ret != SASL_OK);
  DIE_IF(!outputlen || !output);

  ssize_t len = send(conn, output, outputlen, 0);
  DIE_IF(len <= 0);

  return;
 fail:
  close(conn);
  exit(1);
}

void recv_decode(sasl_conn_t *sconn, int conn,
		 const char **data, unsigned *datalen) {
  ssize_t len = recv(conn, buf, HUGE - 1, 0);
  int ret = sasl_decode(sconn, buf, len, data, datalen);
  if (ret == SASL_TOOWEAK) {
    /* see note at encode_send */
    *data = buf;
    *datalen = len;
  } else DIE_IF(ret != SASL_OK);
  return;
 fail:
  close(conn);
  exit(1);
}
