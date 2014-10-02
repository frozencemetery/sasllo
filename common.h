#ifndef ____FILE____
#define ____FILE____

#define _GNU_SOURCE

#include <stdio.h>

#include <sasl/sasl.h>

#define SERVICE "sasllo"
#define HUGE 4096

char buf[HUGE];

#define DIE_IF(c)					  \
  if (c) {						  \
    fprintf(stderr, "Error in function %s at line %d!\n", \
	    __FUNCTION__, __LINE__);			  \
    goto fail;						  \
  }

void encode_send(sasl_conn_t *sconn, int conn,
		 const char *data, unsigned datalen);
void recv_decode(sasl_conn_t *sconn, int conn,
		 const char **data, unsigned *datalen);

#endif /* !____FILE____ */
