#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "test_saba_utils.h"
#include "test_saba_message_queue.h"


/*
 * runner
 */

int main() {
  CU_ErrorCode err;

  CU_TestInfo utils_tests[] = {
    { "'ARRAY_SIZE' test", test_array_size },
    { "'container_of' test", test_container_of },
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
    CU_TEST_INFO_NULL,
  };

  CU_SuiteInfo suites[] = {
    { "utils tests", NULL, NULL, utils_tests },
    { "message & message queue tests", NULL, NULL, msg_q_tests },
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

