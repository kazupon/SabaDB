/*
 * SabaDB: server module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_server.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "saba_utils.h"
#include "saba_message.h"


/*
 * internal defines
 */

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
  saba_message_t *msg;
} write_req_t;


/*
 * event handlers
 */

static void on_server_log(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret) {
  TRACE("logger=%p, level=%d, ret=%d\n", logger, level, ret);
}

static void on_close(uv_handle_t *handle, int status) {
  TRACE("handle=%p, status=%d\n", handle, status);
  free(handle);
}

static void on_after_shutdown(uv_shutdown_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);
  uv_close((uv_handle_t *)req->handle, on_close);
  free(req);
}

static uv_buf_t on_alloc(uv_handle_t *handle, size_t suggested_size) {
  TRACE("handle=%p, suggested_size=%ld\n", handle, suggested_size);
  return uv_buf_init(malloc(suggested_size), suggested_size);
}

static void on_after_write(uv_write_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);
  assert(req != NULL);

  saba_server_t *server = (saba_server_t *)req->data;
  assert(server != NULL);

  write_req_t *wr = (write_req_t *)req;
  if (status) {
    uv_err_t err = uv_last_error(req->handle->loop);
    SABA_LOGGER_LOG(
      server->logger, req->handle->loop, on_server_log, ERROR,
      "uv_write error: %s\n", uv_strerror(err)
    );
    abort();
  }

  /* release */
  free(wr->buf.base);
  free(wr);
}

static void on_after_read(uv_stream_t *peer, ssize_t nread, uv_buf_t buf) {
  TRACE("peer=%p, nread=%zd, buf=%p\n", peer, nread, &buf);
  assert(peer != NULL);

  uv_loop_t *loop = peer->loop;

  if (nread < 0) {
    assert(uv_last_error(loop).code == UV_EOF);
    if (buf.base) {
      free(buf.base);
    }
    uv_shutdown_t *req = (uv_shutdown_t *)malloc(sizeof(uv_shutdown_t));
    assert(req != NULL);
    uv_shutdown(req, peer, on_after_shutdown);
    return;
  }
  assert(peer->data != NULL);

  saba_server_t *server = container_of(peer->data, saba_server_t, tcp);
  assert(server != NULL && server->master != NULL);
  
  /* create request message */
  saba_message_t *msg = saba_message_alloc();
  assert(msg != NULL);
  msg->kind = SABA_MESSAGE_KIND_ECHO;
  msg->stream = peer;
  msg->data = strdup(buf.base);
  TRACE("create request message: kind=%d, stream=%p, data=%p\n", msg->kind, msg->stream, msg->data);

  /* pput request message */
  saba_master_put_request(server->master, msg);

  if (buf.base) {
    free(buf.base);
  }
}

static void on_response(saba_master_t *master, saba_message_t *msg) {
  TRACE("master=%p, msg=%p\n", master, msg);

  write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
  assert(wr != NULL);
  TRACE("wr=%p\n", wr);
  wr->req.data = master;

  wr->buf = uv_buf_init(strdup((const char *)msg->data), strlen((const char *)msg->data));
  int32_t ret = uv_write(&wr->req, msg->stream, &wr->buf, 1, on_after_write);
  if (ret) {
    uv_err_t err = uv_last_error(master->loop);
    SABA_LOGGER_LOG(
      master->logger, master->loop, on_server_log, ERROR,
      "uv_write error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
}

static void on_connection(uv_stream_t *tcp, int status) {
  TRACE("tcp=%p, status=%d\n", tcp, status);
  assert(tcp != NULL);

  uv_loop_t *loop = tcp->loop;
  assert(loop != NULL);

  saba_server_t *server = container_of(tcp, saba_server_t, tcp);
  assert(server != NULL);

  if (status != 0) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      server->logger, loop, on_server_log, ERROR,
      "connect error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  assert(status == 0);

  uv_stream_t *peer = (uv_stream_t *)malloc(sizeof(uv_tcp_t));
  assert(peer != NULL);
  TRACE("peer=%p\n", peer);

  int32_t ret = uv_tcp_init(loop, (uv_tcp_t *)peer);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      server->logger, loop, on_server_log, ERROR,
      "peer init error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  assert(ret == 0);

  peer->data = tcp;

  ret = uv_accept(tcp, peer);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      server->logger, loop, on_server_log, ERROR,
      "accept error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  assert(ret == 0);

  ret = uv_read_start(peer, on_alloc, on_after_read);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      server->logger, loop, on_server_log, ERROR,
      "read start error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  assert(ret == 0);
}



/*
 * server implements
 */

saba_server_t* saba_server_alloc(int32_t worker_num, saba_logger_t *logger) {
  saba_server_t *server = (saba_server_t *)malloc(sizeof(saba_server_t));
  assert(server != NULL);
  TRACE("server=%p\n", server);

  server->logger = logger;
  server->loop = NULL;
  server->db = NULL;
  server->master = saba_master_alloc(worker_num);
  assert(server->master != NULL);
  server->master->logger = logger;

  return server;
}

void saba_server_free(saba_server_t *server) {
  assert(server != NULL);
  TRACE("server=%p\n", server);

  server->logger = NULL;
  server->loop = NULL;
  server->db = NULL;
  saba_master_free(server->master);
  server->master = NULL;
  free(server);
}

saba_err_t saba_server_start(
  saba_server_t *server, uv_loop_t *loop, const char *address, uint16_t port) {
  assert(server != NULL && loop != NULL);
  TRACE("server=%p\n", server);
  
  int32_t ret = uv_tcp_init(loop, &server->tcp);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("socket create error: %s (%d)\n", uv_strerror(err), err.code);
    return SABA_ERR_NG;
  }

  struct sockaddr_in addr = uv_ip4_addr(address, port);
  ret = uv_tcp_bind(&server->tcp, addr);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("bind error: %s (%d)\n", uv_strerror(err), err.code);
    return SABA_ERR_NG;
  }

  ret = uv_listen((uv_stream_t *)&server->tcp, SOMAXCONN, on_connection);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("listen error: %s (%d)\n", uv_strerror(err), err.code);
    return SABA_ERR_NG;
  }

  ret = (int32_t)saba_master_start(server->master, loop, on_response, NULL);
  if (ret) {
    SABA_LOGGER_LOG(
      server->logger, loop, on_server_log, ERROR,
      "master start error: %d\n", ret
    );
    return SABA_ERR_NG;
  }

  server->loop = loop;

  return (saba_err_t)ret;
}

saba_err_t saba_server_stop(saba_server_t *server) {
  assert(server != NULL);
  TRACE("server=%p\n", server);

  saba_err_t ret = saba_master_stop(server->master);
  if (ret) {
    SABA_LOGGER_LOG(
      server->logger, server->loop, on_server_log, ERROR,
      "master master error: %d\n", ret
    );
  }

  uv_close((uv_handle_t *)&server->tcp, NULL);

  return ret;
}

