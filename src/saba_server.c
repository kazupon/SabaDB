/*
 * SabaDB: server module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_server.h"
#include "saba_worker.h"
#include "debug.h"
#include "saba_utils.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>


typedef struct {
  uv_write_t req;
  uv_buf_t buf;
  saba_message_t *msg;
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
  assert(req != NULL);

  write_req_t *wr = (write_req_t *)req;
  if (status) {
    uv_err_t err = uv_last_error(req->handle->loop);
    TRACE("uv_write error: %s\n", uv_strerror(err));
    assert(0);
  }

  saba_server_t *server = (saba_server_t *)req->data;
  assert(server != NULL);

  saba_message_t *msg = (saba_message_t *)wr->msg;
  assert(msg != NULL && msg->stream != NULL && msg->data != NULL);

  /* release */
  free(msg->data);
  msg->data = NULL;
  msg->stream = NULL;
  free(msg);
  free(wr->buf.base);
  wr->msg = NULL;
  free(wr);
}

void on_after_read(uv_stream_t *peer, ssize_t nread, uv_buf_t buf) {
  TRACE("peer=%p, nread=%zd, buf=%p, buf.base=%s\n", peer, nread, &buf, buf.base);
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
  assert(server != NULL && server->req_queue != NULL);
  
  /* create message */
  saba_message_t *msg = (saba_message_t *)malloc(sizeof(saba_message_t));
  assert(msg != NULL);
  msg->kind = SABA_MESSAGE_KIND_ECHO;
  msg->stream = peer;
  msg->data = strdup(buf.base);
  TRACE("create request message: kind=%d, stream=%p, data=%p\n", msg->kind, msg->stream, msg->data);

  saba_message_queue_lock(server->req_queue);

  /* insert message to request message queue */
  saba_message_queue_insert_tail(server->req_queue, msg);
  TRACE("put request message\n");

  ngx_queue_t *q = ngx_queue_head(&server->workers);
  assert(q != NULL);
  ngx_queue_remove(q);
  saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
  assert(worker != NULL);
  int32_t ret = uv_async_send(&worker->req_proc_notifier);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("request proc notifier error: %s (%d)\n", uv_strerror(err), err.code);
  }
  ngx_queue_insert_tail(&server->workers, &worker->q);

  saba_message_queue_unlock(server->req_queue);

  if (buf.base) {
    free(buf.base);
  }
}

uv_buf_t on_alloc(uv_handle_t *handle, size_t suggested_size) {
  TRACE("handle=%p, suggested_size=%ld\n", handle, suggested_size);
  return uv_buf_init(malloc(suggested_size), suggested_size);
}

void on_connection(uv_stream_t *tcp, int status) {
  TRACE("tcp=%p, status=%d\n", tcp, status);
  assert(tcp != NULL);

  uv_loop_t *loop = tcp->loop;
  assert(loop != NULL);

  if (status != 0) {
    uv_err_t err = uv_last_error(loop);
    TRACE("connect error: %s (%d)\n", uv_strerror(err), err.code);
    assert(0);
  }
  assert(status == 0);

  uv_stream_t *peer = (uv_stream_t *)malloc(sizeof(uv_tcp_t));
  assert(peer != NULL);
  TRACE("peer=%p\n", peer);

  int32_t ret = uv_tcp_init(loop, (uv_tcp_t *)peer);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("peer init error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(ret == 0);

  peer->data = tcp;

  ret = uv_accept(tcp, peer);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("accept error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(ret == 0);

  ret = uv_read_start(peer, on_alloc, on_after_read);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    TRACE("read start error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
  assert(ret == 0);
}

void on_watch_res_queue(uv_idle_t *watcher, int status) {
  assert(watcher != NULL && status == 0);
  TRACE("watcher=%p, status=%d\n", watcher, status);

  saba_server_t *server = container_of(watcher, saba_server_t, res_queue_watcher);
  assert(server != NULL && server->res_queue != NULL);

  saba_message_queue_lock(server->res_queue);

  /* check queue */
  if (saba_message_queue_is_empty(server->res_queue)) {
    TRACE("respose message is nothing in response message queue\n");
    int32_t ret = uv_idle_stop(&server->res_queue_watcher);
    if (ret) {
      uv_err_t err = uv_last_error(watcher->loop);
      TRACE("stop response queue watching: %s (%d)\n", uv_strerror(err), err.code);
    }
    saba_message_queue_unlock(server->res_queue);
    return;
  }

  /* get message */
  saba_message_t *msg = saba_message_queue_head(server->res_queue);
  if (msg == NULL || msg->stream == NULL) {
    TRACE("get reponse message is null or 'stream' is null\n");
    saba_message_queue_unlock(server->res_queue);
    return;
  }
  TRACE("get response message: kind=%d, stream=%p, data=%p\n", msg->kind, msg->stream, msg->data);

  /* remove messge from respone message queue */
  saba_message_queue_remove(server->res_queue, msg);

  saba_message_queue_unlock(server->res_queue);

  write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
  assert(wr != NULL);
  TRACE("wr=%p\n", wr);
  wr->msg = msg;
  wr->req.data = server;

  wr->buf = uv_buf_init(strdup((const char *)msg->data), strlen((const char *)msg->data));
  int32_t ret = uv_write(&wr->req, msg->stream, &wr->buf, 1, on_after_write);
  if (ret) {
    uv_err_t err = uv_last_error(watcher->loop);
    TRACE("uv_write error: %s (%d)\n", uv_strerror(err), err.code);
    return;
  }
}

void on_notify_req_proc_done(uv_async_t *notifier, int status) {
  assert(notifier != NULL && status == 0);
  TRACE("notifier=%p, status=%d\n", notifier, status);

  saba_server_t *server = container_of(notifier, saba_server_t, req_proc_done_notifier);
  assert(server != NULL && server->res_queue != NULL);

  saba_message_queue_lock(server->res_queue);

  /* check queue */
  if (!saba_message_queue_is_empty(server->res_queue)) {
    TRACE("respose message in response message queue.\n");
    int32_t ret = uv_idle_start(&server->res_queue_watcher, on_watch_res_queue);
    if (ret) {
      uv_err_t err = uv_last_error(notifier->loop);
      TRACE("start response queue watching: %s (%d)\n", uv_strerror(err), err.code);
    }
  }

  saba_message_queue_unlock(server->res_queue);
}


/*
 * saba_server_t implements
 */

saba_server_t* saba_server_alloc(int32_t worker_num) {
  saba_server_t *server = (saba_server_t *)malloc(sizeof(saba_server_t));
  assert(server != NULL);
  TRACE("server=%p, tcp=%p\n", server, &server->tcp);

  server->req_queue = saba_message_queue_alloc();
  server->res_queue = saba_message_queue_alloc();
  assert(server->req_queue != NULL && server->res_queue != NULL);
  TRACE("req_queue=%p, res_queue=%p\n", server->req_queue, server->res_queue);

  int32_t i = 0;
  ngx_queue_init(&server->workers);
  for (i = 0; i < worker_num; i++) {
    saba_worker_t *worker = saba_worker_alloc();
    worker->master = server;
    worker->req_queue = server->req_queue;
    worker->res_queue = server->res_queue;
    ngx_queue_insert_tail(&server->workers, &worker->q);
  }

  assert(server != NULL);
  return server;
}

void saba_server_free(saba_server_t *server) {
  assert(server != NULL);
  TRACE("server=%p\n", server);

  while (!ngx_queue_empty(&server->workers)) {
    ngx_queue_t *q = ngx_queue_head(&server->workers);
    assert(q != NULL);
    ngx_queue_remove(q);
    saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
    assert(worker != NULL);
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

  ngx_queue_t *q;
  ngx_queue_foreach(q, &server->workers) {
    saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
    saba_err_t err = saba_worker_start(worker);
    TRACE("start worker: err=%d\n", err);
  }

  return (saba_err_t)ret;
}

saba_err_t saba_server_stop(saba_server_t *server) {
  assert(server != NULL);
  TRACE("server=%p\n", server);

  int32_t ret = 0;
  ngx_queue_t *q;
  ngx_queue_foreach(q, &server->workers) {
    saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
    saba_err_t err = saba_worker_stop(worker);
    ret = err;
    TRACE("stop worker: err=%d\n", err);
  }

  uv_close((uv_handle_t *)&server->tcp, NULL);

  uv_unref((uv_handle_t *)&server->res_queue_watcher);
  ret = uv_idle_stop(&server->res_queue_watcher);
  if (ret) {
    uv_err_t err = uv_last_error(&server->req_proc_done_notifier.loop);
    TRACE("stop response queue watcher error: %s (%d)\n", uv_strerror(err), err.code);
  }
  uv_close((uv_handle_t *)&server->res_queue_watcher, NULL);

  uv_unref((uv_handle_t *)&server->req_proc_done_notifier);
  uv_close((uv_handle_t *)&server->req_proc_done_notifier, NULL);

  return (saba_err_t)ret;
}

