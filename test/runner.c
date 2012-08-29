#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "test_saba_utils.h"
#include "test_saba_message_queue.h"
#include "test_saba_logger.h"
#include "test_saba_worker.h"
#include "test_saba_master.h"
#include "test_saba_server.h"


/*
 * runner
 */

int main() {
  CU_ErrorCode err;

  CU_TestInfo utils_tests[] = {
    { "'ARRAY_SIZE' test", test_array_size },
    { "'container_of' test", test_container_of },
    { "'time' test", test_saba_time },
    { "'jetlag' test", test_saba_jetlag },
    { "'date_www_format' test", test_saba_date_www_format },
    CU_TEST_INFO_NULL,
  };

  CU_TestInfo msg_q_tests[] = {
    { "message 'alloc' and 'free' test", test_saba_message_alloc_and_free },
    { "message queue 'alloc', and 'free' test", test_saba_message_queue_alloc_and_free },
    { "message queue 'is_empty' test", test_saba_message_queue_is_empty },
    { "message queue 'head' test", test_saba_message_queue_head },
    { "message queue 'tail' test", test_saba_message_queue_tail },
    { "message queue 'insert_head' test", test_saba_message_queue_insert_head },
    { "message queue 'insert_tail' test", test_saba_message_queue_insert_tail },
    { "message queue 'remove' test", test_saba_message_queue_remove },
    { "message queue 'clear' test", test_saba_message_queue_clear },
    { "message queue 'lock' and 'unlock' test", test_saba_message_queue_lock_and_unlock },
    CU_TEST_INFO_NULL,
  };

  CU_TestInfo logger_tests[] = {
    { "logger 'alloc' and 'free' test", test_saba_logger_alloc_and_free },
    { "logger 'open' with specific path and 'close' test", test_saba_logger_open_with_specific_path_and_close },
    { "logger 'open' with already specific path and 'close' test", test_saba_logger_open_with_already_specific_path_and_close },
    { "logger sync 'open' and 'close' test", test_saba_logger_sync_open_and_close },
    { "logger 'log' test", test_saba_logger_log },
    { "logger sync 'log' test", test_saba_logger_sync_log },
    { "logger 'log' test by thread", test_saba_logger_log_by_thread },
    { "logger macro test", test_saba_logger_macro },
    CU_TEST_INFO_NULL,
  };

  CU_TestInfo worker_tests[] = {
    { "worker 'alloc' and 'free' test", test_saba_worker_alloc_and_free },
    { "worker 'start' and 'stop' test", test_saba_worker_start_and_stop },
    { "worker 'request' callback test", test_saba_worker_on_request },
    CU_TEST_INFO_NULL,
  };

  CU_TestInfo master_tests[] = {
    { "master 'alloc' and 'free' test", test_saba_master_alloc_and_free },
    { "master 'start' and 'stop' test", test_saba_master_start_and_stop },
    { "master 'put_request' test", test_saba_master_put_request },
    { "master 'response' callback test", test_saba_master_on_response },
    { "master 100 'put_request' and 'response' callback test", test_saba_master_100_put_request_and_on_response },
    CU_TEST_INFO_NULL,
  };

  CU_TestInfo server_tests[] = {
    { "server 'alloc' and 'free' test", test_saba_server_alloc_and_free },
    { "server 'start' and 'stop' test", test_saba_server_start_and_stop },
    { "server echo test", test_saba_server_echo },
    CU_TEST_INFO_NULL,
  };

  CU_SuiteInfo suites[] = {
    { "utils tests", NULL, NULL, utils_tests },
    { "message & message queue tests", NULL, NULL, msg_q_tests },
    { "logger tests", NULL, NULL, logger_tests },
    { "worker tests", test_saba_worker_setup, test_saba_worker_teardown, worker_tests },
    { "master tests", test_saba_master_setup, test_saba_master_teardown, master_tests },
    { "server tests", test_saba_server_setup, test_saba_server_teardown, server_tests },
    CU_SUITE_INFO_NULL,
  };

  CU_initialize_registry();
  err = CU_register_suites(suites);
  /*
  suite = CU_add_suite("utilities", setup, teardown);
  suite = CU_add_suite("utilities", NULL, NULL);
  CU_add_test(suite, "test_add", test_add);
  CU_ADD_TEST(suite, test_array_size);
  CU_ADD_TEST(suite, test_container_of);
  CU_ADD_TEST(suite, test_saba_message_alloc_and_free);
  CU_ADD_TEST(suite, test_saba_message_queue_alloc_and_free);
  CU_ADD_TEST(suite, test_saba_message_queue_is_empty);
  CU_ADD_TEST(suite, test_saba_message_queue_head);
  CU_ADD_TEST(suite, test_saba_message_queue_tail);
  CU_ADD_TEST(suite, test_saba_message_queue_insert_head);
  CU_ADD_TEST(suite, test_saba_message_queue_insert_tail);
  CU_ADD_TEST(suite, test_saba_message_queue_remove);
  CU_ADD_TEST(suite, test_saba_message_queue_clear);

  CU_console_run_tests();
  */

  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}

