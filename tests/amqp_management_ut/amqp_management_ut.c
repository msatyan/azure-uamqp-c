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
#include "umocktypes_stdint.h"
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
#include "azure_c_shared_utility/singlylinkedlist.h"
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
static SINGLYLINKEDLIST_HANDLE test_singlylinkedlist_handle = (SINGLYLINKEDLIST_HANDLE)0x4243;
static AMQP_VALUE test_source_amqp_value = (AMQP_VALUE)0x4244;
static AMQP_VALUE test_target_amqp_value = (AMQP_VALUE)0x4245;
static LINK_HANDLE test_sender_link = (LINK_HANDLE)0x4246;
static LINK_HANDLE test_receiver_link = (LINK_HANDLE)0x4247;
static MESSAGE_SENDER_HANDLE test_message_sender = (MESSAGE_SENDER_HANDLE)0x4248;
static MESSAGE_RECEIVER_HANDLE test_message_receiver = (MESSAGE_RECEIVER_HANDLE)0x424A;

MOCK_FUNCTION_WITH_CODE(, void, test_amqp_management_open_complete, void*, context, AMQP_MANAGEMENT_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

#define role_VALUES \
    role_sender,    \
    role_receiver

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(role, role_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(role, role_VALUES);

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

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(role, role);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_RETURN(singlylinkedlist_create, test_singlylinkedlist_handle);
    REGISTER_GLOBAL_MOCK_RETURN(messaging_create_source, test_source_amqp_value);
    REGISTER_GLOBAL_MOCK_RETURN(messaging_create_target, test_target_amqp_value);
    REGISTER_GLOBAL_MOCK_RETURN(messagesender_create, test_message_sender);
    REGISTER_GLOBAL_MOCK_RETURN(messagereceiver_create, test_message_receiver);
    
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MANAGEMENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LINK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SENDER_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVER_STATE_CHANGED, void*);
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

/* Tests_SRS_AMQP_MANAGEMENT_01_001: [ `amqp_management_create` shall create a new CBS instance and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_003: [ `amqp_management_create` shall create a singly linked list for pending operations by calling `singlylinkedlist_create`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_006: [ `amqp_management_create` shall create a sender link by calling `link_create`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_010: [ The `source` argument shall be a value created by calling `messaging_create_source` with `management_node` as argument. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_011: [ The `target` argument shall be a value created by calling `messaging_create_target` with `management_node` as argument. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_015: [ `amqp_management_create` shall create a receiver link by calling `link_create`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_022: [ `amqp_management_create` shall create a message sender by calling `messagesender_create` and passing to it the sender link handle. ]*/
TEST_FUNCTION(amqp_management_create_returns_a_valid_handle)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(singlylinkedlist_create());
    STRICT_EXPECTED_CALL(messaging_create_source("test_node"));
    STRICT_EXPECTED_CALL(messaging_create_target("test_node"));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(link_create(test_session_handle, "test_node-sender", role_sender, test_source_amqp_value, test_target_amqp_value))
        .SetReturn(test_sender_link);
    STRICT_EXPECTED_CALL(link_create(test_session_handle, "test_node-receiver", role_receiver, test_source_amqp_value, test_target_amqp_value))
        .SetReturn(test_receiver_link);;
    STRICT_EXPECTED_CALL(messagesender_create(test_sender_link, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagereceiver_create(test_sender_link, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    amqp_management = amqp_management_create(test_session_handle, "test_node");

    // assert
    ASSERT_IS_NOT_NULL(amqp_management);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_002: [ If `session` or `management_node` is NULL then `amqp_management_create` shall fail and return NULL. ]*/
TEST_FUNCTION(amqp_management_create_with_NULL_session_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;

    // act
    amqp_management = amqp_management_create(NULL, "test_node");

    // assert
    ASSERT_IS_NULL(amqp_management);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AMQP_MANAGEMENT_01_002: [ If `session` or `management_node` is NULL then `amqp_management_create` shall fail and return NULL. ]*/
TEST_FUNCTION(amqp_management_create_with_NULL_management_node_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;

    // act
    amqp_management = amqp_management_create(test_session_handle, NULL);

    // assert
    ASSERT_IS_NULL(amqp_management);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(amqp_management_ut)
