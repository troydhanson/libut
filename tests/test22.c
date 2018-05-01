#include <stdio.h>
#include "ringbuf.h"

int main() {
  int sc, rc = -1;
  size_t len;
  char *data;

  ringbuf *r = NULL;
  r = ringbuf_new(10);
  if (r == NULL) goto done;

  sc = ringbuf_put(r, "hello", 5);
  if (sc < 0) goto done;

  len = ringbuf_get_next_chunk(r, &data);
  assert(len == 5);
  printf("next chunk length : %zu\n", len);
  printf("%.*s\n", (int)len, data);
  ringbuf_mark_consumed(r, len);

  /* now there are 10 bytes free but only 5 contiguous */
  len = ringbuf_get_writable(r, &data);
  if (len) printf("writable space: %zu\n", len);
  assert(len == 5);
  memcpy(data, "abcde", 5);
  ringbuf_wrote(r, len);

  /* now there are 5 bytes free, starting at wrap */
  len = ringbuf_get_writable(r, &data);
  if (len) printf("writable space: %zu\n", len);
  assert(len == 5);
  memcpy(data, "fghij", 5);
  ringbuf_wrote(r, len);

  /* read abcde */
  len = ringbuf_get_next_chunk(r, &data);
  assert(len == 5);
  printf("next chunk length : %zu\n", len);
  printf("%.*s\n", (int)len, data);
  ringbuf_mark_consumed(r, len);

  /* read fghji after the wrap */
  len = ringbuf_get_next_chunk(r, &data);
  assert(len == 5);
  printf("next chunk length : %zu\n", len);
  printf("%.*s\n", (int)len, data);
  ringbuf_mark_consumed(r, len);

 done:
  if (r) ringbuf_free(r);
  return rc;
}
