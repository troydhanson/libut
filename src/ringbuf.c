#include "ringbuf.h"

ringbuf *ringbuf_new(size_t sz) {
  ringbuf *r = malloc(sizeof(*r) + sz);
  if (r == NULL) {
    fprintf(stderr,"out of memory\n");
    goto done;
  }

  r->u = r->i = r->o = 0;
  r->n = sz;

 done:
  return r;
}

void ringbuf_free(ringbuf* r) {
  free(r);
}

/* ringbuf_take: alternative to ringbuf_new; caller 
 * provides the buffer to use as the ringbuf.
 * buffer should be aligned e.g. from malloc/mmap.
 * note: do not ringbuf_free afterward. 
 */
#define MIN_RINGBUF (sizeof(ringbuf) + 1)
ringbuf *ringbuf_take(void *buf, size_t sz) {
  if (sz < MIN_RINGBUF) return NULL;
  ringbuf *r = (ringbuf*)buf;

  r->u = r->i = r->o = 0;
  r->n = sz - sizeof(*r); // alignment should be ok
  assert(r->n > 0);

  return r;
}


/* copy data in. fails if ringbuf has insuff space. */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
int ringbuf_put(ringbuf *r, const void *_data, size_t len) {
  char *data = (char*)_data;
  size_t a,b,c;
  if (r->i < r->o) {  // available space is a contiguous buffer
    a = r->o - r->i; 
    assert(a == r->n - r->u);
    if (len > a) return -1;
    memcpy(&r->d[r->i], data, len);
  } else {            // available space wraps; it's two buffers
    b = r->n - r->i;  // in-head to eob (receives leading input)
    c = r->o;         // out-head to in-head (receives trailing input)
    a = b + c;        // available space
    // the only ambiguous case is i==o, that's why u is needed
    if (r->i == r->o) a = r->n - r->u; 
    assert(a == r->n - r->u);
    if (len > a) return -1;
    memcpy(&r->d[r->i], data, MIN(b, len));
    if (len > b) memcpy(r->d, &data[b], len-b);
  }
  r->i = (r->i + len) % r->n;
  r->u += len;
  return 0;
}

size_t ringbuf_get_freespace(ringbuf *r) {
  return r->n - r->u;
}

size_t ringbuf_get_pending_size(ringbuf *r) {
  return r->u;
}

size_t ringbuf_get_next_chunk(ringbuf *r, char **data) {
  // in this case the next chunk is the whole pending buffer
  if (r->o < r->i) {
    assert(r->u == r->i - r->o);
    *data = &r->d[r->o];
    return r->u;
  }
  // in this case (i==o) either the buffer is empty of full.
  // r->u tells distinguishes these cases.
  if ((r->o == r->i) && (r->u == 0)) { *data=NULL; return 0; }
  // if we're here, that means r->o > r->i. the pending
  // output is wrapped around the buffer. this function 
  // returns the chunk prior to eob. the next call gets
  // the next chunk that's wrapped around the buffer.
  size_t b,c;
  b = r->n - r->o; // length of the part we're returning
  c = r->i;        // wrapped part length- a sanity check
  assert(r->u == b + c);
  *data = &r->d[r->o];
  return b;
}

void ringbuf_mark_consumed(ringbuf *r, size_t len) {
  assert(len <= r->u);
  r->o = (r->o + len ) % r->n;
  r->u -= len;
}

void ringbuf_clear(ringbuf *r) {
  r->u = r->i = r->o = 0;
}

/*
 * ringbuf_get_writable
 *
 * get a pointer into the next chunk of writable space
 * whose length is returned in *len. If callers writes
 * into this space, it must be followed with a call to
 * ringbuf_wrote
 */
size_t ringbuf_get_writable(ringbuf *r, char **data) {

  /* any room in ring? */
  if (r->n == r->u) {
    *data = NULL;
    return 0;
  }

  if (r->i == r->o) {
    assert(r->u == 0);
  }

  *data = r->d + r->i;

  return (r->i >= r->o) ?
    r->n - r->i : /* free space is from r->i to end */
    r->o - r->i;  /* free space is from r->i to r->o */

}

/* used after ringbuf_get_writable to indicate len
 * bytes were written. this can only be used with
 * a len less than, or equal to, the result from 
 * that function
 */
void ringbuf_wrote(ringbuf *r, size_t len) {

  size_t max = (r->i >= r->o) ?
    r->n - r->i : /* free space is from r->i to end */
    r->o - r->i;  /* free space is from r->i to r->o */

  assert(len <= max);

  r->u += len;
  r->i = (r->i + len) % r->n;
}

