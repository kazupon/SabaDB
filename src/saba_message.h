/*
 * SabaDB: message module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_MESSAGE_H
#define SABA_MESSAGE_H


#include <uv.h>


/*
 * message kind
 */
typedef enum {
  SABA_MESSAGE_KIND_UNKNOWN,
  SABA_MESSAGE_KIND_ECHO,
} saba_message_kind_t;

/*
 * message
 */
typedef struct {
  saba_message_kind_t kind;
  uv_stream_t *stream;
  void* data;
  ngx_queue_t q; /* private */
} saba_message_t;


/*
 * message prototype(s)
 */
saba_message_t* saba_message_alloc(void);
void saba_message_free(saba_message_t *msg);


#endif /* SABA_MESSAGE_H */

