/*
 * SabaDB: message queue module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_message_queue.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>


saba_message_queue_t* saba_message_queue_alloc(void) {
  saba_message_queue_t *msg_q = (saba_message_queue_t *)malloc(sizeof(saba_message_queue_t));
  assert(msg_q != NULL);

  ngx_queue_init(&msg_q->queue);
  uv_mutex_init(&msg_q->mtx);

  return msg_q;
}

void saba_message_queue_free(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);

  saba_message_queue_lock(msg_q);
  saba_message_queue_clear(msg_q);
  saba_message_queue_unlock(msg_q);

  uv_mutex_destroy(&msg_q->mtx);
  free(msg_q);
}

bool saba_message_queue_is_empty(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);

  return ngx_queue_empty(&msg_q->queue);
}

saba_message_t* saba_message_queue_head(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);

  if (saba_message_queue_is_empty(msg_q)) {
    return NULL;
  }
  
  ngx_queue_t *head = ngx_queue_head(&msg_q->queue);
  assert(head != NULL);
  saba_message_t *msg = ngx_queue_data(head, saba_message_t, q);
  assert(msg != NULL);

  return msg;
}

saba_message_t* saba_message_queue_tail(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);

  if (saba_message_queue_is_empty(msg_q)) {
    return NULL;
  }

  ngx_queue_t *tail = ngx_queue_last(&msg_q->queue);
  assert(tail!= NULL);
  saba_message_t *msg = ngx_queue_data(tail, saba_message_t, q);
  assert(msg != NULL);

  return msg;
}

void saba_message_queue_insert_head(saba_message_queue_t *msg_q, saba_message_t *msg) {
  assert(msg_q != NULL && msg != NULL);

  ngx_queue_insert_head(&msg_q->queue, &msg->q);
}

void saba_message_queue_insert_tail(saba_message_queue_t *msg_q, saba_message_t *msg) {
  assert(msg_q != NULL && msg != NULL);

  ngx_queue_insert_tail(&msg_q->queue, &msg->q);
}

void saba_message_queue_remove(saba_message_queue_t *msg_q, saba_message_t *msg) {
  assert(msg_q != NULL && msg != NULL);

  ngx_queue_t *q;
  saba_message_t *w = NULL;
  ngx_queue_foreach(q, &msg_q->queue) {
    w = ngx_queue_data(q, saba_message_t, q);
  }
  ngx_queue_remove(&msg->q);
}

void saba_message_queue_clear(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);

  if (!saba_message_queue_is_empty(msg_q)) {
    ngx_queue_t *q;
    saba_message_t *msg = NULL;
    ngx_queue_foreach(q, &msg_q->queue) {
      msg = ngx_queue_data(q, saba_message_t, q);
      assert(msg != NULL);
      ngx_queue_remove(&msg->q);
    }
  }
}

void saba_message_queue_lock(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  uv_mutex_lock(&msg_q->mtx);
}

void saba_message_queue_unlock(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  uv_mutex_unlock(&msg_q->mtx);
}

