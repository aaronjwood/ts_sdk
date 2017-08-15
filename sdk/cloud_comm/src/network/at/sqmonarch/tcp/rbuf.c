#include <string.h>
#include "rbuf.h"

struct rbuf {
	volatile size_t ridx;
	volatile size_t widx;
	size_t sz;
	uint8_t *data;
};

static rbuf rbufs[MAX_RING_BUFS];
static bool rbuf_used[MAX_RING_BUFS];

static size_t find_unused_rbuf(void)
{
	for (size_t i = 0; i < MAX_RING_BUFS; i++)
		if (!rbuf_used[i])
			return i;
	return SIZE_MAX;
}

rbuf *rbuf_init(size_t len, uint8_t buffer[])
{
	size_t idx = find_unused_rbuf();
	if (idx == SIZE_MAX || buffer == NULL)
		return NULL;
	rbuf_used[idx] = true;
	rbufs[idx].sz = len;
	rbufs[idx].data = buffer;
	rbuf_clear(&rbufs[idx]);
	return &rbufs[idx];
}

bool rbuf_wb(rbuf *r, uint8_t data)
{
	if (r == NULL)
		return false;
	if (rbuf_unread(r) == r->sz)
		return false;
	const size_t idx = r->widx % r->sz;
	r->data[idx] = data;
	r->widx++;
	return true;
}

bool rbuf_rb(rbuf *r, uint8_t *data)
{
	if (r == NULL || data == NULL)
		return false;
	if (rbuf_unread(r) == 0)
		return false;
	const size_t idx = r->ridx % r->sz;
	*data = r->data[idx];
	r->ridx++;
	return true;
}

size_t rbuf_unread(const rbuf * const r)
{
	if (r == NULL)
		return 0;
	return r->widx - r->ridx;
}

bool rbuf_clear(rbuf *r)
{
	if (r == NULL)
		return false;

	r->ridx = 0;
	r->widx = 0;
	memset(r->data, 0, r->sz);
	return true;
}
