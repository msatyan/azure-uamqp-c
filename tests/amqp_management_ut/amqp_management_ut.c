// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdbool>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif
#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umock_c_negative_tests.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS

#include "azure_c_shared_utility/gballoc.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/amqpvalue_to_string.h"

#undef ENABLE_MOCKS

#include "azure_uamqp_c/amqp_management.h"

static SESSION_HANDLE test_session_handle = (SESSION_HANDLE)0x4242;

MOCK_FUNCTION_WITH_CODE(, void, test_amqp_management_open_complete, void*, context, AMQP_MANAGEMENT_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(amqp_management_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MANAGEMENT_HANDLE, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(test_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(test_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* amqp_management_create */

TEST_FUNCTION(amqp_management_create_returns_a_valid_handle)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    // act
    amqp_management = amqp_management_create(test_session_handle, "test_node");

    // assert
    ASSERT_IS_NOT_NULL(amqp_management);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

END_TEST_SUITE(amqp_management_ut)
