#ifndef TEST_SABA_LOGGER_H
#define TEST_SABA_LOGGER_H

#include "saba_logger.h"


void test_saba_logger_alloc_and_free(void);
void test_saba_logger_open_with_specific_path_and_close(void);
void test_saba_logger_open_with_already_specific_path_and_close(void);
void test_saba_logger_sync_open_and_close(void);

void test_saba_logger_log(void);
void test_saba_logger_sync_log(void);
void test_saba_logger_log_by_thread(void);

#endif /* TEST_SABA_LOGGER_H */

