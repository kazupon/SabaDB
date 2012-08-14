/*
 * SabaDB: server module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_server.h"
#include "saba_worker.h"
#include "debug.h"
#include "saba_utils.h"
#include <assert.h>
#include <stdlib.h>


typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;


/*
 * event handlers
 */

void on_close(uv_handle_t *handle, int status) {
  TRACE("handle=%p, status=%d\n", handle, status);
  free(handle);
}

void on_after_shutdown(uv_shutdown_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);
  uv_close((uv_handle_t *)req->handle, on_close);
  free(req);
}

void on_after_write(uv_write_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);

  write_req_t *wr = (write_req_t *)req;
  if (status) {
    uv_err_t err = uv_last_error(req->handle->loop);
    TRACE("uv_write error: %s\n", uv_strerror(err));
    assert(0);
  }

  free(wr->buf.base);
  free(wr);
}

void on_after_read(uv_stream_t *handle, ssize_t nread, uv_buf_t buf) {
  TRACE("handle=%p, nread=%zd, buf=%p, buf.base=%s\n", handle, nread, &buf, buf.base);

  uv_loop_t *loop = handle->loop;

  if (nread < 0) {
    assert(uv_last_error(loop).code == UV_EOF);
    if (buf.base) {
      free(buf.base);
    }
    uv_shutdown_t *req = (uv_shutdown_t *)malloc(sizeof(uv_shutdown_t));
    assert(req != NULL);
    uv_shutdown(req, handle, on_after_shutdown);
    return;
  }

  write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
  assert(wr != NULL);
  TRACE("wr=%p\n", wr);

  assert(handle->data != NULL);
  saba_server_t *server = container_of(handle->data, saba_server_t, tcp);
  assert(server != NULL);
  TRACE("server=%p\n", server);
  
  /* create message */
  saba_message_t *msg = (saba_message_t *)malloc(sizeof(saba_message_t));
  assert(msg != NULL);
  msg->data = "hello"; /* TODO: copy !! */
  TRACE("message: kind=%d, data=%p\n", msg->kind, msg->data);

  /* check queue */
  bool is_empty = saba_message_queue_is_empty(server->req_queue);

  saba_message_queue_insert_tail(server->req_queue, msg);

  if (is_empty) {
    int32_t i = 0;
    for (i = 0; i < ARRAY_SIZE(server->workers); i++) {
      int32_t ret = uv_async_send(&server->workers[i]->req_proc_notifier);
      if (ret) {
        uv_err_t err = uv_last_error(loop);
        TRACE("request proc notifier error: %s (%d)\n", uv_strerror(err), err.code);
      }
    }
  }

  wr->buf = uv_buf_init(buf.base, nread);
  int32_t ret = uv_write(&wr->req, handle, &wr->buf, 1, on_after_write);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("uv_write error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
}

uv_buf_t on_alloc(uv_handle_t *handle, size_t suggested_size) {
  TRACE("handle=%p, suggested_size=%ld\n", handle, suggested_size);
  return uv_buf_init(malloc(suggested_size), suggested_size);
}

void on_connection(uv_stream_t *tcp, int status) {
  assert(tcp != NULL);
  TRACE("tcp=%p, status=%d\n", tcp, status);

  uv_loop_t *loop = tcp->loop;
  assert(loop != NULL);

  if (status != 0) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Connect error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(status == 0);

  uv_stream_t *peer = (uv_stream_t *)malloc(sizeof(uv_tcp_t));
  assert(peer != NULL);
  TRACE("peer=%p\n", peer);

  int32_t ret = uv_tcp_init(loop, (uv_tcp_t *)peer);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Peer init error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(ret == 0);

  peer->data = tcp;

  ret = uv_accept(tcp, peer);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Accept error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(ret == 0);

  ret = uv_read_start(peer, on_alloc, on_after_read);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Read start error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(ret == 0);
}

void on_watch_res_queue(uv_idle_t *watcher, int status) {
  assert(watcher != NULL && status == 0);
  TRACE("watcher=%p, status=%d\n", watcher, status);

  saba_server_t *server = container_of(watcher, saba_server_t, res_queue_watcher);
  assert(server != NULL);
  TRACE("server=%p\n", server);

  /* check queue */
  if (saba_message_queue_is_empty(server->res_queue)) {
    TRACE("nothing respose item in response queue.\n");
    int32_t ret = uv_idle_stop(&server->res_queue_watcher);
    if (ret) {
      uv_err_t err = uv_last_error(watcher->loop);
      TRACE("stop response queue watching: %s (%d)\n", uv_strerror(err), err.code);
    }
    return;
  }

  /* get message */
  saba_message_t *msg = saba_message_queue_head(server->res_queue);
  assert(msg != NULL);
  TRACE("kind: %d\n", msg->kind);
  saba_message_queue_remove(server->req_queue, msg);

  /* release message */
  /* TODO:
  free(msg->command);
  free(msg);
  */
  free(msg);
}

void on_notify_req_proc_done(uv_async_t *notifier, int status) {
  assert(notifier != NULL && status == 0);
  TRACE("notifier=%p, status=%d\n", notifier, status);

  saba_server_t *server = container_of(notifier, saba_server_t, req_proc_done_notifier);
  assert(server != NULL);
  TRACE("server=%p\n", server);

  /* check queue */
  if (!saba_message_queue_is_empty(server->res_queue)) {
    TRACE("respose item in response queue.\n");
    int32_t ret = uv_idle_start(&server->res_queue_watcher, on_watch_res_queue);
    if (ret) {
      uv_err_t err = uv_last_error(notifier->loop);
      TRACE("start response queue watching: %s (%d)\n", uv_strerror(err), err.code);
    }
  }
}


/*
 * saba_server_t implements
 */

saba_server_t* saba_server_alloc() {
  saba_server_t *server = (saba_server_t *)malloc(sizeof(saba_server_t));
  assert(server != NULL);
  TRACE("server=%p, tcp=%p\n", server, &server->tcp);

  server->req_queue = saba_message_queue_alloc();
  server->res_queue = saba_message_queue_alloc();
  assert(server->req_queue != NULL && server->res_queue != NULL);
  TRACE("req_queue=%p, res_queue=%p\n", server->req_queue, server->res_queue);

  int32_t members = 2;
  server->workers = (saba_server_t **)malloc(sizeof(saba_server_t *) * members);
  assert(server->workers != NULL);

  int32_t i = 0;
  for (i = 0; i < ARRAY_SIZE(server->workers); i++) {
    server->workers[i] = saba_worker_alloc();
    assert(server->workers[i]);
    TRACE("create worker: %p\n", server->workers[i]);
    server->workers[i]->master = server;
    server->workers[i]->req_queue = server->req_queue;
    server->workers[i]->res_queue = server->res_queue;
  }

  assert(server != NULL);
  return server;
}

void saba_server_free(saba_server_t *server) {
  assert(server != NULL);
  TRACE("server=%p\n", server);

  int32_t i = 0;
  for (i = 0; i < ARRAY_SIZE(server->workers); i++) {
    saba_worker_t *worker = server->workers[i];
    worker->master = NULL;
    worker->req_queue = NULL;
    worker->res_queue = NULL;
    saba_worker_free(worker);
  }

  saba_message_queue_free(server->req_queue);
  saba_message_queue_free(server->res_queue);
  server->req_queue = NULL;
  server->res_queue = NULL;
}

saba_err_t saba_server_start(
  saba_server_t *server, uv_loop_t *loop, const char *address, uint16_t port) {
  assert(server != NULL && loop != NULL);
  TRACE("server=%p\n", server);
  
  int32_t ret = uv_idle_init(loop, &server->res_queue_watcher);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("response queue watcher init error: %s (%d)\n", uv_strerror(err), err.code);
    /* TODO: should be released saba_server_t resource */
    return SABA_ERR_NG;
  }

  ret = uv_idle_start(&server->res_queue_watcher, on_watch_res_queue);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("watch response queue start error: %s (%d)\n", uv_strerror(err), err.code);
    /* TODO: should be released saba_server_t resource */
    return SABA_ERR_NG;
  }

  uv_ref((uv_handle_t *)&server->res_queue_watcher);

  ret = uv_async_init(loop, &server->req_proc_done_notifier, on_notify_req_proc_done);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("request want notifier init error: %s (%d)\n", uv_strerror(err), err.code);
    /* TODO: should be released saba_server_t resource */
    return SABA_ERR_NG;
  }

  uv_ref((uv_handle_t *)&server->req_proc_done_notifier);

  ret = uv_tcp_init(loop, &server->tcp);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Socket create error: %s (%d)\n", uv_strerror(err), err.code);
    return SABA_ERR_NG;
  }

  struct sockaddr_in addr = uv_ip4_addr(address, port);
  ret = uv_tcp_bind(&server->tcp, addr);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Bind error: %s (%d)\n", uv_strerror(err), err.code);
    return SABA_ERR_NG;
  }

  ret = uv_listen((uv_stream_t *)&server->tcp, SOMAXCONN, on_connection);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("Listen error: %s (%d)\n", uv_strerror(err), err.code);
    return SABA_ERR_NG;
  }

  int32_t i = 0;
  for (i = 0; i < ARRAY_SIZE(server->workers); i++) {
    assert(server->workers[i]);
    saba_err_t err = saba_worker_start(server->workers[i]);
    TRACE("start worker: err=%d\n", err);
  }

  return (saba_err_t)ret;
}

saba_err_t saba_server_stop(saba_server_t *server) {
  assert(server != NULL);
  TRACE("server=%p\n", server);

  int32_t i = 0;
  int32_t ret = 0;
  for (i = 0; i < ARRAY_SIZE(server->workers); i++) {
    assert(server->workers[i]);
    saba_err_t err = saba_worker_stop(server->workers[i]);
    ret = err;
    TRACE("stop worker: err=%d\n", err);
  }

  uv_unref((uv_handle_t *)&server->tcp);
  uv_close((uv_handle_t *)&server->tcp, NULL);

  uv_unref((uv_handle_t *)&server->res_queue_watcher);
  uv_close((uv_handle_t *)&server->res_queue_watcher, NULL);

  uv_unref((uv_handle_t *)&server->req_proc_done_notifier);
  uv_close((uv_handle_t *)&server->req_proc_done_notifier, NULL);

  return (saba_err_t)ret;
}

