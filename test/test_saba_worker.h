#ifndef TEST_SABA_WORKER_H
#define TEST_SABA_WORKER_H


#include "saba_worker.h"


int test_saba_worker_setup(void);
int test_saba_worker_teardown(void);
void test_saba_worker_alloc_and_free(void);
void test_saba_worker_start_and_stop(void);
void test_saba_worker_on_request(void);


#endif /* TEST_SABA_WORKER_H */

