#include <stdio.h>
#include "ringbuf.h"

int main() {
  int rc = -1;
  size_t len;
  char *data;

  ringbuf *r = NULL;
  r = ringbuf_new(10);
  if (r == NULL) goto done;

  len = ringbuf_get_writable(r, &data);
  if (len) printf("writable space: %zu\n", len);

  assert(len == 10);
  memcpy(data, "abcdefghik", 10);
  ringbuf_wrote(r, len);

  len = ringbuf_get_writable(r, &data);
  if (len) printf("writable space: %zu\n", len);
  assert(len == 0);

  len = ringbuf_get_next_chunk(r, &data);
  assert(len == 10);
  printf("next chunk length : %zu\n", len);
  printf("%.*s\n", (int)len, data);
  ringbuf_mark_consumed(r, len);

  len = ringbuf_get_next_chunk(r, &data);
  assert(len == 0);
  printf("next chunk length : %zu\n", len);

 done:
  if (r) ringbuf_free(r);
  return rc;
}
