#ifndef TEST_SABA_MESSAGE_QUEUE_H
#define TEST_SABA_MESSAGE_QUEUE_H


#include "saba_message.h"
#include "saba_message_queue.h"


void test_saba_message_alloc_and_free(void);

void test_saba_message_queue_alloc_and_free(void);
void test_saba_message_queue_is_empty(void);
void test_saba_message_queue_head(void);
void test_saba_message_queue_tail(void);
void test_saba_message_queue_insert_head(void);
void test_saba_message_queue_insert_tail(void);
void test_saba_message_queue_remove(void);
void test_saba_message_queue_clear(void);
void test_saba_message_queue_lock_and_unlock(void);

#endif /* TEST_SABA_MESSAGE_QUEUE_H */

