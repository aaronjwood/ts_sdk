/**
 * \file rbuf.h
 * \copyright Copyright (C) 2017 Verizon. All rights reserved.
 * \brief Single producer, single consumer ring buffer API.
 */

#ifndef RBUF_H
#define RBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Maximum number of ring buffers that can be simultaneously be instantiated
 */
#define MAX_RING_BUFS		1

struct rbuf;

/**
 * @brief Opaque data type describing the ring buffer.
 */
typedef struct rbuf rbuf;

rbuf *rbuf_init(size_t len, uint8_t buffer[]);

bool rbuf_wb(rbuf *r, uint8_t data);

bool rbuf_rb(rbuf *r, uint8_t *data);

size_t rbuf_unread(const rbuf * const r);

bool rbuf_clear(rbuf *r);
#endif
