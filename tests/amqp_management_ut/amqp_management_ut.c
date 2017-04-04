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
static MESSAGE_HANDLE test_message = (MESSAGE_HANDLE)0x424B;
static MESSAGE_HANDLE test_cloned_message = (MESSAGE_HANDLE)0x424C;
static AMQP_VALUE test_application_properties = (AMQP_VALUE)0x4301;
static AMQP_VALUE test_operation_key = (AMQP_VALUE)0x4302;
static AMQP_VALUE test_operation_value = (AMQP_VALUE)0x4303;
static AMQP_VALUE test_type_key = (AMQP_VALUE)0x4304;
static AMQP_VALUE test_type_value = (AMQP_VALUE)0x4305;
static AMQP_VALUE test_locales_key = (AMQP_VALUE)0x4306;
static AMQP_VALUE test_locales_value = (AMQP_VALUE)0x4307;
static AMQP_VALUE test_message_id_value = (AMQP_VALUE)0x4308;
static AMQP_VALUE test_string_value = (AMQP_VALUE)0x4309;
static PROPERTIES_HANDLE test_properties = (PROPERTIES_HANDLE)0x430A;

MOCK_FUNCTION_WITH_CODE(, void, test_amqp_management_open_complete, void*, context, AMQP_MANAGEMENT_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

#define role_VALUES \
    role_sender,    \
    role_receiver

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#ifndef __cplusplus
TEST_DEFINE_ENUM_TYPE(role, role_VALUES);
#endif
IMPLEMENT_UMOCK_C_ENUM_TYPE(role, role_VALUES);

MOCK_FUNCTION_WITH_CODE(, void, test_on_amqp_management_open_complete, void*, context, AMQP_MANAGEMENT_OPEN_RESULT, open_result);
MOCK_FUNCTION_END();
MOCK_FUNCTION_WITH_CODE(, void, test_on_amqp_management_error, void*, context);
MOCK_FUNCTION_END();
MOCK_FUNCTION_WITH_CODE(, void, test_on_amqp_management_execute_operation_complete, void*, context, AMQP_MANAGEMENT_EXECUTE_OPERATION_RESULT, execute_operation_result, unsigned int, status_code, const char*, status_description)
MOCK_FUNCTION_END();

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
    REGISTER_GLOBAL_MOCK_RETURN(link_create, test_sender_link);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_message_id_ulong, test_message_id_value);
    REGISTER_GLOBAL_MOCK_RETURN(message_get_application_properties, 0);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, test_string_value);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_set_map_value, 0);
    REGISTER_GLOBAL_MOCK_RETURN(message_set_application_properties, 0);
    REGISTER_GLOBAL_MOCK_RETURN(message_get_properties, 0);
    REGISTER_GLOBAL_MOCK_RETURN(message_set_properties, 0);
    REGISTER_GLOBAL_MOCK_RETURN(properties_set_message_id, 0);
    REGISTER_GLOBAL_MOCK_RETURN(message_clone, test_cloned_message);

    REGISTER_UMOCK_ALIAS_TYPE(AMQP_MANAGEMENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SINGLYLINKEDLIST_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LINK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_SENDER_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVER_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_SENDER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_RECEIVER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PROPERTIES_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(message_id_ulong, uint64_t);

    REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(link_create, link_destroy);
    REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(messagesender_create, messagesender_destroy);
    REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(messagereceiver_create, messagereceiver_destroy);
    REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(singlylinkedlist_create, singlylinkedlist_destroy);
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
/* Tests_SRS_AMQP_MANAGEMENT_01_023: [ `amqp_management_create` shall create a message receiver by calling `messagereceiver_create` and passing to it the receiver link handle. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_007: [ The `session` argument shall be set to `session`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_008: [ The `name` argument shall be constructed by concatenating the `management_node` value with `-sender`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_009: [ The `role` argument shall be `role_sender`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_016: [ The `session` argument shall be set to `session`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_017: [ The `name` argument shall be constructed by concatenating the `management_node` value with `-receiver`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_018: [ The `role` argument shall be `role_receiver`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_019: [ The `source` argument shall be the value created by calling `messaging_create_source`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_020: [ The `target` argument shall be the value created by calling `messaging_create_target`. ]*/
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
        .SetReturn(test_receiver_link);
    STRICT_EXPECTED_CALL(messagesender_create(test_sender_link, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagereceiver_create(test_receiver_link, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));

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

/* Tests_SRS_AMQP_MANAGEMENT_01_030: [ If `management_node` is an empty string, then `amqp_management_create` shall fail and return NULL. ]*/
TEST_FUNCTION(amqp_management_create_with_empty_string_for_management_node_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;

    // act
    amqp_management = amqp_management_create(test_session_handle, "");

    // assert
    ASSERT_IS_NULL(amqp_management);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AMQP_MANAGEMENT_01_004: [ If `singlylinkedlist_create` fails, `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_005: [ If allocating memory for the new handle fails, `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_012: [ If `messaging_create_source` fails then `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_013: [ If `messaging_create_target` fails then `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_014: [ If `link_create` fails when creating the sender link then `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_021: [ If `link_create` fails when creating the receiver link then `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_031: [ If `messagesender_create` fails then `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_032: [ If `messagereceiver_create` fails then `amqp_management_create` shall fail and return NULL. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_033: [ If any other error occurs `amqp_management_create` shall fail and return NULL. ]*/
TEST_FUNCTION(when_any_undelying_function_call_fails_amqp_management_create_fails)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    size_t count;
    size_t index;
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(singlylinkedlist_create())
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(messaging_create_source("test_node"))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(messaging_create_target("test_node"))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(link_create(test_session_handle, "test_node-sender", role_sender, test_source_amqp_value, test_target_amqp_value))
        .SetReturn(test_sender_link)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(link_create(test_session_handle, "test_node-receiver", role_receiver, test_source_amqp_value, test_target_amqp_value))
        .SetReturn(test_receiver_link)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(messagesender_create(test_sender_link, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(messagereceiver_create(test_receiver_link, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    count = umock_c_negative_tests_call_count();
    for (index = 0; index < count - 4; index++)
    {
        char tmp_msg[128];
        AMQP_MANAGEMENT_HANDLE amqp_management;
        (void)sprintf(tmp_msg, "Failure in test %u/%u", (unsigned int)(index + 1), (unsigned int)count);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        // act
        amqp_management = amqp_management_create(test_session_handle, "test_node");

        // assert
        ASSERT_IS_NULL_WITH_MSG(amqp_management, tmp_msg);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

/* amqp_management_destroy */

/* Tests_SRS_AMQP_MANAGEMENT_01_024: [ `amqp_management_destroy` shall free all the resources allocated by `amqp_management_create`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_026: [ `amqp_management_destroy` shall free the singly linked list by calling `singlylinkedlist_destroy`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_027: [ `amqp_management_destroy` shall free the sender and receiver links by calling `link_destroy`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_028: [ `amqp_management_destroy` shall free the message sender by calling `messagesender_destroy`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_029: [ `amqp_management_destroy` shall free the message receiver by calling `messagereceiver_destroy`. ]*/
TEST_FUNCTION(amqp_management_destroy_frees_all_the_allocated_resources)
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
        .SetReturn(test_receiver_link);
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagesender_destroy(test_message_sender));
    STRICT_EXPECTED_CALL(messagereceiver_destroy(test_message_receiver));
    STRICT_EXPECTED_CALL(link_destroy(test_sender_link));
    STRICT_EXPECTED_CALL(link_destroy(test_receiver_link));
    STRICT_EXPECTED_CALL(singlylinkedlist_destroy(test_singlylinkedlist_handle));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    amqp_management_destroy(amqp_management);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AMQP_MANAGEMENT_01_025: [ If `amqp_management` is NULL, `amqp_management_destroy` shall do nothing. ]*/
TEST_FUNCTION(amqp_management_destroy_with_NULL_handle_does_nothing)
{
    // arrange

    // act
    amqp_management_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* amqp_management_open_async */

/* Tests_SRS_AMQP_MANAGEMENT_01_036: [ `amqp_management_open_async` shall start opening the AMQP management instance and save the callbacks so that they can be called when opening is complete. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_037: [ On success it shall return 0. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_039: [ `amqp_management_open_async` shall open the message sender by calling `messagesender_open`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_040: [ `amqp_management_open_async` shall open the message receiver by calling `messagereceiver_open`. ]*/
TEST_FUNCTION(amqp_management_open_async_opens_the_message_sender_and_message_receiver)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagereceiver_open(test_message_receiver, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagesender_open(test_message_sender));

    // act
    result = amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_044: [ `on_amqp_management_open_complete_context` and `on_amqp_management_error_context` shall be allowed to be NULL. ]*/
TEST_FUNCTION(amqp_management_open_async_with_NULL_context_arguments_opens_the_message_sender_and_message_receiver)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagereceiver_open(test_message_receiver, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagesender_open(test_message_sender));

    // act
    result = amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, NULL, test_on_amqp_management_error, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_042: [ If `messagereceiver_open` fails, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_opening_the_receiver_fails_amqp_management_open_async_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagereceiver_open(test_message_receiver, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(1);

    // act
    result = amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_041: [ If `messagesender_open` fails, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_opening_the_sender_fails_amqp_management_open_async_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagereceiver_open(test_message_receiver, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(messagesender_open(test_message_sender))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(messagereceiver_close(test_message_receiver));

    // act
    result = amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_038: [ If `amqp_management`, `on_amqp_management_open_complete` or `on_amqp_management_error` is NULL, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(amqp_management_open_async_with_NULL_handle_fails)
{
    // arrange
    int result;

    // act
    result = amqp_management_open_async(NULL, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AMQP_MANAGEMENT_01_038: [ If `amqp_management`, `on_amqp_management_open_complete` or `on_amqp_management_error` is NULL, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(amqp_management_open_async_with_NULL_open_complete_callback_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    // act
    result = amqp_management_open_async(amqp_management, NULL, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_038: [ If `amqp_management`, `on_amqp_management_open_complete` or `on_amqp_management_error` is NULL, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(amqp_management_open_async_with_NULL_error_complete_callback_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    umock_c_reset_all_calls();

    // act
    result = amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_043: [ If the AMQP management instance is already OPEN or OPENING, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(amqp_management_open_async_when_opening_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    (void)amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);
    umock_c_reset_all_calls();

    // act
    result = amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* amqp_management_close */

/* Tests_SRS_AMQP_MANAGEMENT_01_045: [ `amqp_management_close` shall close the AMQP management instance. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_046: [ On success it shall return 0. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_050: [ `amqp_management_close` shall close the message sender by calling `messagesender_close`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_051: [ `amqp_management_close` shall close the message receiver by calling `messagereceiver_close`. ]*/
TEST_FUNCTION(amqp_management_close_closes_the_message_sender_and_message_receiver)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    (void)amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagesender_close(test_message_sender));
    STRICT_EXPECTED_CALL(messagereceiver_close(test_message_receiver));

    // act
    result = amqp_management_close(amqp_management);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_047: [ If `amqp_management` is NULL, `amqp_management_close` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(amqp_management_close_with_NULL_handle_fails)
{
    // arrange
    int result;

    // act
    result = amqp_management_close(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_AMQP_MANAGEMENT_01_052: [ If `messagesender_close` fails, `amqp_management_close` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_closing_the_sender_fails_amqp_management_close_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    (void)amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagesender_close(test_message_sender))
        .SetReturn(1);

    // act
    result = amqp_management_close(amqp_management);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* Tests_SRS_AMQP_MANAGEMENT_01_053: [ If `messagereceiver_close` fails, `amqp_management_close` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_closing_the_receiver_fails_amqp_management_close_fails)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    (void)amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagesender_close(test_message_sender))
        .SetReturn(1);

    // act
    result = amqp_management_close(amqp_management);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

/* amqp_management_execute_operation_async */

/* Tests_SRS_AMQP_MANAGEMENT_01_055: [ `amqp_management_execute_operation_async` shall start an AMQP management operation. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_056: [ On success it shall return 0. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_082: [ `amqp_management_execute_operation_async` shall obtain the application properties from the message by calling `message_get_application_properties`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_084: [ For each of the arguments `operation`, `type` and `locales` an AMQP value of type string shall be created by calling `amqpvalue_create_string` in order to be used as key in the application properties map. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_085: [ For each of the arguments `operation`, `type` and `locales` an AMQP value of type string containing the argument value shall be created by calling `amqpvalue_create_string` in order to be used as value in the application properties map. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_086: [ The key/value pairs for `operation`, `type` and `locales` shall be added to the application properties map by calling `amqpvalue_set_map_value`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_087: [ The application properties obtained after adding the key/value pairs shall be set on the message by calling `message_set_application_properties`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_088: [ `amqp_management_execute_operation_async` shall send the message by calling `messagesender_send`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_094: [ In order to set the message Id on the message, the properties shall be obtained by calling `message_get_properties`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_095: [ A message Id with the next ulong value to be used shall be created by calling `amqpvalue_create_message_id_ulong`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_096: [ The message Id value shall be set on the properties by calling `properties_set_message_id`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_097: [ The properties thus modified to contain the message Id shall be set on the message by calling `message_set_properties`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_100: [ After setting the properties, the properties instance shall be freed by `properties_destroy`. ]*/
/* Tests_SRS_AMQP_MANAGEMENT_01_091: [ Once the request message has been sent, an entry shall be stored in the pending operations list by calling `singlylinkedlist_add`. ]*/
TEST_FUNCTION(amqp_management_execute_operation_async_starts_the_operation)
{
    // arrange
    AMQP_MANAGEMENT_HANDLE amqp_management;
    int result;
    amqp_management = amqp_management_create(test_session_handle, "test_node");
    (void)amqp_management_open_async(amqp_management, test_on_amqp_management_open_complete, (void*)0x4242, test_on_amqp_management_error, (void*)0x4243);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(message_get_application_properties(test_message, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_application_properties(&test_application_properties, sizeof(test_application_properties));
    STRICT_EXPECTED_CALL(amqpvalue_create_string("operation"))
        .SetReturn(test_operation_key);
    STRICT_EXPECTED_CALL(amqpvalue_create_string("some_operation"))
        .SetReturn(test_operation_value);
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(test_application_properties, test_operation_key, test_operation_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_operation_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_operation_key));
    STRICT_EXPECTED_CALL(amqpvalue_create_string("type"))
        .SetReturn(test_type_key);
    STRICT_EXPECTED_CALL(amqpvalue_create_string("some_type"))
        .SetReturn(test_type_value);
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(test_application_properties, test_type_key, test_type_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_type_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_type_key));
    STRICT_EXPECTED_CALL(amqpvalue_create_string("locales"))
        .SetReturn(test_locales_key);
    STRICT_EXPECTED_CALL(amqpvalue_create_string("en-US"))
        .SetReturn(test_locales_value);
    STRICT_EXPECTED_CALL(amqpvalue_set_map_value(test_application_properties, test_locales_key, test_locales_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_locales_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_locales_key));
    STRICT_EXPECTED_CALL(message_set_application_properties(test_message, test_application_properties));
    STRICT_EXPECTED_CALL(message_get_properties(test_message, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_properties(&test_properties, sizeof(test_properties));
    STRICT_EXPECTED_CALL(amqpvalue_create_message_id_ulong(0));
    STRICT_EXPECTED_CALL(properties_set_message_id(test_properties, test_message_id_value));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_message_id_value));
    STRICT_EXPECTED_CALL(message_set_properties(test_message, test_properties));
    STRICT_EXPECTED_CALL(properties_destroy(test_properties));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(message_clone(test_message));
    STRICT_EXPECTED_CALL(singlylinkedlist_add(test_singlylinkedlist_handle, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(amqpvalue_destroy(test_application_properties));

    // act
    result = amqp_management_execute_operation_async(amqp_management, "some_operation", "some_type", "en-US", test_message, test_on_amqp_management_execute_operation_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    amqp_management_destroy(amqp_management);
}

END_TEST_SUITE(amqp_management_ut)
