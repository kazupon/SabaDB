#ifndef TEST_SABA_MASTER_H
#define TEST_SABA_MASTER_H


#include "saba_master.h"


int test_saba_master_setup(void);
int test_saba_master_teardown(void);
void test_saba_master_alloc_and_free(void);
void test_saba_master_start_and_stop(void);
void test_saba_master_put_request(void);
void test_saba_master_on_response(void);
void test_saba_master_100_put_request_and_on_response(void);


#endif /* TEST_SABA_MASTER_H */

