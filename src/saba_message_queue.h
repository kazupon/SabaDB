/*
 * SabaDB: message queue module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_MESSAGE_QUEUE_H
#define SABA_MESSAGE_QUEUE_H


#include <stdbool.h>
#include <uv.h>
#include "saba_message.h"


/*
 * message queue
 */

typedef struct {
  ngx_queue_t queue; /* private */
  uv_mutex_t mtx; /* private */
} saba_message_queue_t;


/*
 * message queue prototypes
 */

saba_message_queue_t* saba_message_queue_alloc(void);
void saba_message_queue_free(saba_message_queue_t *msg_q);
bool saba_message_queue_is_empty(saba_message_queue_t *msg_q);
saba_message_t* saba_message_queue_head(saba_message_queue_t *msg_q);
saba_message_t* saba_message_queue_tail(saba_message_queue_t *msg_q);
void saba_message_queue_insert_head(saba_message_queue_t *msg_q, saba_message_t *msg);
void saba_message_queue_insert_tail(saba_message_queue_t *msg_q, saba_message_t *msg);
void saba_message_queue_remove(saba_message_queue_t *msg_q, saba_message_t *msg);
void saba_message_queue_clear(saba_message_queue_t *msg_q);
void saba_message_queue_lock(saba_message_queue_t *msg_q);
void saba_message_queue_unlock(saba_message_queue_t *msg_q);


#endif /* SABA_MESSAGE_QUEUE_H */

