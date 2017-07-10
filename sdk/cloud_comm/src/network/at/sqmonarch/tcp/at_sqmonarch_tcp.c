/* Copyright (C) 2017 Verizon. All rights reserved. */
#include "at_tcp.h"
#include "at_sqmonarch_tcp_command.h"

bool at_init()
{
	return true;
}

int at_tcp_connect(const char *host, const char *port)
{
	return 0;
}

int at_tcp_send(int s_id, const uint8_t *buf, size_t len)
{
	return 0;
}

int at_read_available(int s_id)
{
	return 0;
}

int at_tcp_recv(int s_id, uint8_t *buf, size_t len)
{
	return 0;
}

void at_tcp_close(int s_id)
{
	/* Code */
}
