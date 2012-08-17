/*
 * SabaDB: roundtrips benchmark
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in ../src/main.h
 */

#include "debug.h"
#include "saba_utils.h"
#include <stdlib.h>
#include <assert.h>
#include <uv.h>


typedef struct {
  uv_tcp_t tcp;
  uv_connect_t connection;
  uint64_t count;
} echo_t;

static int64_t TIME = 5000;
static int64_t start_time;

uv_buf_t on_alloc_buf(uv_handle_t *tcp, size_t size) {
  TRACE("tcp=%p, size=%zd\n", tcp, size);
  return uv_buf_init(malloc(size), size);
}

void on_write(uv_write_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);
  free(req);
}

void on_read(uv_stream_t *tcp, ssize_t nread, uv_buf_t buf) {
  TRACE("tcp=%p, nread=%zd, buf=%p, buf.base=%s\n", tcp, nread, &buf, buf.base);
  echo_t *echo = container_of(tcp, echo_t, tcp);

  if (nread < 0) {
    assert(uv_last_error(tcp->loop).code == UV_EOF);
    uv_close((uv_handle_t *)tcp, NULL);
    return;
  }

  if (!strcmp("hello", buf.base)) {
    echo->count++;
    if (uv_now(tcp->loop) - start_time > TIME) {
      printf("ping-pong: %lld roundtrips/s\n", (1000 * echo->count) / TIME);
      uv_close((uv_handle_t *)tcp, NULL);
    } else {
      uv_buf_t buf;
      buf.base = "hello";
      buf.len = strlen(buf.base);
      uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
      if (uv_write(req, (uv_stream_t *)&echo->tcp, &buf, 1, on_write)) {
        assert("uv_write failed\n");
      }
    }
  }
}

void on_connect(uv_connect_t *connection, int status) {
  TRACE("connection=%p, status=%d\n", connection, status);
  echo_t *echo = container_of(connection, echo_t, connection);

  uv_buf_t buf;
  buf.base = "hello";
  buf.len = strlen(buf.base);
  uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
  if (uv_write(req, (uv_stream_t *)&echo->tcp, &buf, 1, on_write)) {
    assert("uv_write failed\n");
  }

  if (uv_read_start(connection->handle, on_alloc_buf, on_read)) {
    assert("uv_read_start failed\n");
  }
}

int main () {
  uv_loop_t *loop = uv_default_loop();
  start_time = uv_now(loop);

  echo_t echo;
  struct sockaddr_in client_addr = uv_ip4_addr("0.0.0.0", 0);
  struct sockaddr_in server_addr = uv_ip4_addr("127.0.0.1", 1978);
  uv_tcp_init(loop, &echo.tcp);
  uv_tcp_bind(&echo.tcp, client_addr);
  uv_tcp_connect(&echo.connection, &echo.tcp, server_addr, on_connect);
  echo.count = 0;

  int ret = uv_run(loop);

  return 0;
}
