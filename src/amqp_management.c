// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_uamqp_c/amqp_management.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/amqpvalue_to_string.h"

static const char sender_suffix[] = "-sender";
static const char receiver_suffix[] = "-receiver";

#define COUNT_CHARS(str) (sizeof(str) / sizeof((str)[0]) - 1)

typedef enum OPERATION_STATE_TAG
{
    OPERATION_STATE_NOT_SENT,
    OPERATION_STATE_AWAIT_REPLY
} OPERATION_STATE;

typedef struct OPERATION_MESSAGE_INSTANCE_TAG
{
    MESSAGE_HANDLE message;
    OPERATION_STATE operation_state;
    ON_AMQP_MANAGEMENT_EXECUTE_OPERATION_COMPLETE on_execute_operation_complete;
    void* callback_context;
    unsigned long message_id;
} OPERATION_MESSAGE_INSTANCE;

typedef enum AMQP_MANAGEMENT_STATE_TAG
{
    AMQP_MANAGEMENT_STATE_IDLE,
    AMQP_MANAGEMENT_STATE_OPENING,
    AMQP_MANAGEMENT_STATE_OPEN,
    AMQP_MANAGEMENT_STATE_ERROR
} AMQP_MANAGEMENT_STATE;

typedef struct AMQP_MANAGEMENT_INSTANCE_TAG
{
    LINK_HANDLE sender_link;
    LINK_HANDLE receiver_link;
    MESSAGE_SENDER_HANDLE message_sender;
    MESSAGE_RECEIVER_HANDLE message_receiver;
    OPERATION_MESSAGE_INSTANCE** operation_messages;
    SINGLYLINKEDLIST_HANDLE pending_operations;
    size_t operation_message_count;
    unsigned long next_message_id;
    ON_AMQP_MANAGEMENT_OPEN_COMPLETE on_amqp_management_open_complete;
    void* on_amqp_management_open_complete_context;
    ON_AMQP_MANAGEMENT_ERROR on_amqp_management_error;
    void* on_amqp_management_error_context;
    AMQP_MANAGEMENT_STATE amqp_management_state;
    int sender_connected : 1;
    int receiver_connected : 1;
} AMQP_MANAGEMENT_INSTANCE;

static void remove_operation_message_by_index(AMQP_MANAGEMENT_INSTANCE* amqp_management_instance, size_t index)
{
    message_destroy(amqp_management_instance->operation_messages[index]->message);
    free(amqp_management_instance->operation_messages[index]);

    if (amqp_management_instance->operation_message_count - index > 1)
    {
        memmove(&amqp_management_instance->operation_messages[index], &amqp_management_instance->operation_messages[index + 1], sizeof(OPERATION_MESSAGE_INSTANCE*) * (amqp_management_instance->operation_message_count - index - 1));
    }

    if (amqp_management_instance->operation_message_count == 1)
    {
        free(amqp_management_instance->operation_messages);
        amqp_management_instance->operation_messages = NULL;
    }
    else
    {
        OPERATION_MESSAGE_INSTANCE** new_operation_messages = (OPERATION_MESSAGE_INSTANCE**)realloc(amqp_management_instance->operation_messages, sizeof(OPERATION_MESSAGE_INSTANCE*) * (amqp_management_instance->operation_message_count - 1));
        if (new_operation_messages != NULL)
        {
            amqp_management_instance->operation_messages = new_operation_messages;
        }
    }

    amqp_management_instance->operation_message_count--;
}

static AMQP_VALUE on_message_received(const void* context, MESSAGE_HANDLE message)
{
    AMQP_MANAGEMENT_INSTANCE* amqp_management_instance = (AMQP_MANAGEMENT_INSTANCE*)context;

    AMQP_VALUE application_properties;
    if (message_get_application_properties(message, &application_properties) != 0)
    {
        /* error */
    }
    else
    {
        PROPERTIES_HANDLE response_properties;

        if (message_get_properties(message, &response_properties) != 0)
        {
            /* error */
        }
        else
        {
            AMQP_VALUE key;
            AMQP_VALUE value;
            AMQP_VALUE desc_key;
            AMQP_VALUE desc_value;
            AMQP_VALUE map;
            AMQP_VALUE correlation_id_value;

            if (properties_get_correlation_id(response_properties, &correlation_id_value) != 0)
            {
                /* error */
            }
            else
            {
                map = amqpvalue_get_inplace_described_value(application_properties);
                if (map == NULL)
                {
                    /* error */
                }
                else
                {
                    key = amqpvalue_create_string("status-code");
                    if (key == NULL)
                    {
                        /* error */
                    }
                    else
                    {
                        value = amqpvalue_get_map_value(map, key);
                        if (value == NULL)
                        {
                            /* error */
                        }
                        else
                        {
                            int32_t status_code;
                            if (amqpvalue_get_int(value, &status_code) != 0)
                            {
                                /* error */
                            }
                            else
                            {
                                desc_key = amqpvalue_create_string("status-description");
                                if (desc_key == NULL)
                                {
                                    /* error */
                                }
                                else
                                {
                                    const char* status_description = NULL;

                                    desc_value = amqpvalue_get_map_value(map, desc_key);
                                    if (desc_value != NULL)
                                    {
                                        amqpvalue_get_string(desc_value, &status_description);
                                    }

                                    size_t i = 0;
                                    while (i < amqp_management_instance->operation_message_count)
                                    {
                                        if (amqp_management_instance->operation_messages[i]->operation_state == OPERATION_STATE_AWAIT_REPLY)
                                        {
                                            AMQP_VALUE expected_message_id = amqpvalue_create_ulong(amqp_management_instance->operation_messages[i]->message_id);
                                            AMQP_MANAGEMENT_EXECUTE_OPERATION_RESULT execute_operation_result;

                                            if (expected_message_id == NULL)
                                            {
                                                break;
                                            }
                                            else
                                            {
                                                if (amqpvalue_are_equal(correlation_id_value, expected_message_id))
                                                {
                                                    /* 202 is not mentioned in the draft in any way, this is a workaround for an EH bug for now */
                                                    if ((status_code != 200) && (status_code != 202))
                                                    {
                                                        execute_operation_result = AMQP_MANAGEMENT_EXECUTE_OPERATION_FAILED_BAD_STATUS;
                                                    }
                                                    else
                                                    {
                                                        execute_operation_result = AMQP_MANAGEMENT_EXECUTE_OPERATION_OK;
                                                    }

                                                    amqp_management_instance->operation_messages[i]->on_execute_operation_complete(amqp_management_instance->operation_messages[i]->callback_context, execute_operation_result, status_code, status_description);

                                                    remove_operation_message_by_index(amqp_management_instance, i);

                                                    amqpvalue_destroy(expected_message_id);

                                                    break;
                                                }
                                                else
                                                {
                                                    i++;
                                                }

                                                amqpvalue_destroy(expected_message_id);
                                            }
                                        }
                                        else
                                        {
                                            i++;
                                        }
                                    }

                                    if (desc_value != NULL)
                                    {
                                        amqpvalue_destroy(desc_value);
                                    }
                                    amqpvalue_destroy(desc_key);
                                }
                            }
                            amqpvalue_destroy(value);
                        }
                        amqpvalue_destroy(key);
                    }
                }
            }

            properties_destroy(response_properties);
        }

        application_properties_destroy(application_properties);
    }

    return messaging_delivery_accepted();
}

static int send_operation_messages(AMQP_MANAGEMENT_INSTANCE* amqp_management_instance)
{
    int result;

    if ((amqp_management_instance->sender_connected != 0) &&
        (amqp_management_instance->receiver_connected != 0))
    {
        size_t i;

        for (i = 0; i < amqp_management_instance->operation_message_count; i++)
        {
            if (amqp_management_instance->operation_messages[i]->operation_state == OPERATION_STATE_NOT_SENT)
            {
                if (messagesender_send(amqp_management_instance->message_sender, amqp_management_instance->operation_messages[i]->message, NULL, NULL) != 0)
                {
                    /* error */
                    break;
                }

                amqp_management_instance->operation_messages[i]->operation_state = OPERATION_STATE_AWAIT_REPLY;
            }
        }

        if (i < amqp_management_instance->operation_message_count)
        {
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

static void on_message_sender_state_changed(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    AMQP_MANAGEMENT_INSTANCE* amqp_management_instance = (AMQP_MANAGEMENT_INSTANCE*)context;
    (void)previous_state;
    switch (new_state)
    {
    default:
        break;

    case MESSAGE_SENDER_STATE_OPEN:
        switch (amqp_management_instance->amqp_management_state)
        {
        default:
        case AMQP_MANAGEMENT_STATE_ERROR:
            /* do nothing */
            break;

        case AMQP_MANAGEMENT_STATE_OPENING:
            amqp_management_instance->sender_connected = -1;
            if (amqp_management_instance->receiver_connected != 0)
            {
                (void)send_operation_messages(amqp_management_instance);
                amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_OPEN;
                amqp_management_instance->on_amqp_management_open_complete(amqp_management_instance->on_amqp_management_open_complete_context, AMQP_MANAGEMENT_OPEN_OK);
            }
            break;

        case AMQP_MANAGEMENT_STATE_OPEN:
            amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_ERROR;
            amqp_management_instance->on_amqp_management_error(amqp_management_instance->on_amqp_management_error_context);
            break;
        }

        break;

    case MESSAGE_SENDER_STATE_CLOSING:
    case MESSAGE_SENDER_STATE_IDLE:
    case MESSAGE_SENDER_STATE_ERROR:
        amqp_management_instance->sender_connected = 0;
        switch (amqp_management_instance->amqp_management_state)
        {
        default:
        case AMQP_MANAGEMENT_STATE_ERROR:
            /* do nothing */
            break;

        case AMQP_MANAGEMENT_STATE_OPENING:
            amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_IDLE;
            amqp_management_instance->on_amqp_management_open_complete(amqp_management_instance->on_amqp_management_open_complete_context, AMQP_MANAGEMENT_OPEN_ERROR);
            break;

        case AMQP_MANAGEMENT_STATE_OPEN:
            amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_ERROR;
            amqp_management_instance->on_amqp_management_error(amqp_management_instance->on_amqp_management_error_context);
            break;
        }
        break;
    }
}

static void on_message_receiver_state_changed(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    AMQP_MANAGEMENT_INSTANCE* amqp_management_instance = (AMQP_MANAGEMENT_INSTANCE*)context;
    (void)previous_state;
    switch (new_state)
    {
    default:
        break;

    case MESSAGE_RECEIVER_STATE_OPEN:
        switch (amqp_management_instance->amqp_management_state)
        {
        default:
        case AMQP_MANAGEMENT_STATE_ERROR:
            /* do nothing */
            break;

        case AMQP_MANAGEMENT_STATE_OPENING:
            amqp_management_instance->receiver_connected = -1;

            if (amqp_management_instance->sender_connected != 0)
            {
                (void)send_operation_messages(amqp_management_instance);
                amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_OPEN;
                amqp_management_instance->on_amqp_management_open_complete(amqp_management_instance->on_amqp_management_open_complete_context, AMQP_MANAGEMENT_OPEN_OK);
            }
            break;

        case AMQP_MANAGEMENT_STATE_OPEN:
            amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_ERROR;
            amqp_management_instance->on_amqp_management_error(amqp_management_instance->on_amqp_management_error_context);
            break;
        }

        break;

    case MESSAGE_RECEIVER_STATE_CLOSING:
    case MESSAGE_RECEIVER_STATE_IDLE:
    case MESSAGE_RECEIVER_STATE_ERROR:
        amqp_management_instance->sender_connected = 0;
        switch (amqp_management_instance->amqp_management_state)
        {
        default:
        case AMQP_MANAGEMENT_STATE_ERROR:
            /* do nothing */
            break;

        case AMQP_MANAGEMENT_STATE_OPENING:
            amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_IDLE;
            amqp_management_instance->on_amqp_management_open_complete(amqp_management_instance->on_amqp_management_open_complete_context, AMQP_MANAGEMENT_OPEN_ERROR);
            break;

        case AMQP_MANAGEMENT_STATE_OPEN:
            amqp_management_instance->amqp_management_state = AMQP_MANAGEMENT_STATE_ERROR;
            amqp_management_instance->on_amqp_management_error(amqp_management_instance->on_amqp_management_error_context);
            break;
        }
        break;
    }
}

static int set_message_id(MESSAGE_HANDLE message, unsigned long next_message_id)
{
    int result = 0;

    PROPERTIES_HANDLE properties;

    /* Codes_SRS_AMQP_MANAGEMENT_01_094: [ In order to set the message Id on the message, the properties shall be obtained by calling `message_get_properties`. ]*/
    if (message_get_properties(message, &properties) != 0)
    {
        result = __FAILURE__;
    }
    else
    {
        if (properties == NULL)
        {
            properties = properties_create();
        }

        if (properties == NULL)
        {
            result = __FAILURE__;
        }
        else
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_095: [ A message Id with the next ulong value to be used shall be created by calling `amqpvalue_create_message_id_ulong`. ]*/
            AMQP_VALUE message_id = amqpvalue_create_message_id_ulong(next_message_id);
            if (message_id == NULL)
            {
                result = __FAILURE__;
            }
            else
            {
                /* Codes_SRS_AMQP_MANAGEMENT_01_096: [ The message Id value shall be set on the properties by calling `properties_set_message_id`. ]*/
                if (properties_set_message_id(properties, message_id) != 0)
                {
                    result = __FAILURE__;
                }

                amqpvalue_destroy(message_id);
            }

            /* Codes_SRS_AMQP_MANAGEMENT_01_097: [ The properties thus modified to contain the message Id shall be set on the message by calling `message_set_properties`. ]*/
            if (message_set_properties(message, properties) != 0)
            {
                result = __FAILURE__;
            }

            /* Codes_SRS_AMQP_MANAGEMENT_01_100: [ After setting the properties, the properties instance shall be freed by `properties_destroy`. ]*/
            properties_destroy(properties);
        }
    }

    return result;
}

static int add_string_key_value_pair_to_map(AMQP_VALUE map, const char* key, const char* value)
{
    int result;

    /* Codes_SRS_AMQP_MANAGEMENT_01_084: [ For each of the arguments `operation`, `type` and `locales` an AMQP value of type string shall be created by calling `amqpvalue_create_string` in order to be used as key in the application properties map. ]*/
    AMQP_VALUE key_value = amqpvalue_create_string(key);
    if (key_value == NULL)
    {
        result = __FAILURE__;
    }
    else
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_085: [ For each of the arguments `operation`, `type` and `locales` an AMQP value of type string containing the argument value shall be created by calling `amqpvalue_create_string` in order to be used as value in the application properties map. ]*/
        AMQP_VALUE value_value = amqpvalue_create_string(value);
        if (value_value == NULL)
        {
            result = __FAILURE__;
        }
        else
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_086: [ The key/value pairs for `operation`, `type` and `locales` shall be added to the application properties map by calling `amqpvalue_set_map_value`. ]*/
            if (amqpvalue_set_map_value(map, key_value, value_value) != 0)
            {
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }

            amqpvalue_destroy(value_value);
        }

        amqpvalue_destroy(key_value);
    }

    return result;
}

AMQP_MANAGEMENT_HANDLE amqp_management_create(SESSION_HANDLE session, const char* management_node)
{
    AMQP_MANAGEMENT_INSTANCE* result;

    if ((session == NULL) ||
        (management_node == NULL))
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_002: [ If `session` or `management_node` is NULL then `amqp_management_create` shall fail and return NULL. ]*/
        LogError("Bad arguments: session = %p, management_node = %p");
        result = NULL;
    }
    else if (strlen(management_node) == 0)
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_030: [ If `management_node` is an empty string, then `amqp_management_create` shall fail and return NULL. ]*/
        LogError("Empty string management node");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_001: [ `amqp_management_create` shall create a new CBS instance and on success return a non-NULL handle to it. ]*/
        result = (AMQP_MANAGEMENT_INSTANCE*)malloc(sizeof(AMQP_MANAGEMENT_INSTANCE));
        if (result == NULL)
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_005: [ If allocating memory for the new handle fails, `amqp_management_create` shall fail and return NULL. ]*/
            LogError("Cannot allocate memory for AMQP management handle");
        }
        else
        {
            result->sender_connected = 0;
            result->receiver_connected = 0;
            result->operation_message_count = 0;
            result->operation_messages = NULL;
            result->on_amqp_management_open_complete = NULL;
            result->on_amqp_management_open_complete_context = NULL;
            result->on_amqp_management_error = NULL;
            result->on_amqp_management_error_context = NULL;
            result->amqp_management_state = AMQP_MANAGEMENT_STATE_IDLE;

            /* Codes_SRS_AMQP_MANAGEMENT_01_003: [ `amqp_management_create` shall create a singly linked list for pending operations by calling `singlylinkedlist_create`. ]*/
            result->pending_operations = singlylinkedlist_create();
            if (result->pending_operations == NULL)
            {
                /* Codes_SRS_AMQP_MANAGEMENT_01_004: [ If `singlylinkedlist_create` fails, `amqp_management_create` shall fail and return NULL. ]*/
                LogError("Cannot create pending operations list");
                free(result);
                result = NULL;
            }
            else
            {
                /* Codes_SRS_AMQP_MANAGEMENT_01_010: [ The `source` argument shall be a value created by calling `messaging_create_source` with `management_node` as argument. ]*/
                AMQP_VALUE source = messaging_create_source(management_node);
                if (source == NULL)
                {
                    /* Codes_SRS_AMQP_MANAGEMENT_01_012: [ If `messaging_create_source` fails then `amqp_management_create` shall fail and return NULL. ]*/
                    LogError("Failed creating source AMQP value");
                    singlylinkedlist_destroy(result->pending_operations);
                    free(result);
                    result = NULL;
                }
                else
                {
                    /* Codes_SRS_AMQP_MANAGEMENT_01_011: [ The `target` argument shall be a value created by calling `messaging_create_target` with `management_node` as argument. ]*/
                    AMQP_VALUE target = messaging_create_target(management_node);
                    if (target == NULL)
                    {
                        /* Codes_SRS_AMQP_MANAGEMENT_01_013: [ If `messaging_create_target` fails then `amqp_management_create` shall fail and return NULL. ]*/
                        LogError("Failed creating target AMQP value");
                        singlylinkedlist_destroy(result->pending_operations);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        size_t management_node_length = strlen(management_node);

                        char* sender_link_name = (char*)malloc(management_node_length + COUNT_CHARS(sender_suffix) + 1);
                        if (sender_link_name == NULL)
                        {
                            /* Codes_SRS_AMQP_MANAGEMENT_01_033: [ If any other error occurs `amqp_management_create` shall fail and return NULL. ]*/
                            LogError("Failed allocating memory for sender link name");
                            result = NULL;
                        }
                        else
                        {
                            (void)memcpy(sender_link_name, management_node, management_node_length);
                            (void)memcpy(sender_link_name + management_node_length, sender_suffix, COUNT_CHARS(sender_suffix) + 1);

                            char* receiver_link_name = (char*)malloc(management_node_length + COUNT_CHARS(receiver_suffix) + 1);
                            if (receiver_link_name == NULL)
                            {
                                /* Codes_SRS_AMQP_MANAGEMENT_01_033: [ If any other error occurs `amqp_management_create` shall fail and return NULL. ]*/
                                LogError("Failed allocating memory for receiver link name");
                                result = NULL;
                            }
                            else
                            {
                                (void)memcpy(receiver_link_name, management_node, management_node_length);
                                (void)memcpy(receiver_link_name + management_node_length, receiver_suffix, COUNT_CHARS(receiver_suffix) + 1);

                                /* Codes_SRS_AMQP_MANAGEMENT_01_006: [ `amqp_management_create` shall create a sender link by calling `link_create`. ]*/
                                /* Codes_SRS_AMQP_MANAGEMENT_01_007: [ The `session` argument shall be set to `session`. ]*/
                                /* Codes_SRS_AMQP_MANAGEMENT_01_008: [ The `name` argument shall be constructed by concatenating the `management_node` value with `-sender`. ]*/
                                /* Codes_SRS_AMQP_MANAGEMENT_01_009: [ The `role` argument shall be `role_sender`. ]*/
                                /* Codes_SRS_AMQP_MANAGEMENT_01_019: [ The `source` argument shall be the value created by calling `messaging_create_source`. ]*/
                                /* Codes_SRS_AMQP_MANAGEMENT_01_020: [ The `target` argument shall be the value created by calling `messaging_create_target`. ]*/
                                result->sender_link = link_create(session, sender_link_name, role_sender, source, target);
                                if (result->sender_link == NULL)
                                {
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_014: [ If `link_create` fails when creating the sender link then `amqp_management_create` shall fail and return NULL. ]*/
                                    LogError("Failed creating sender link");
                                    free(result);
                                    result = NULL;
                                }
                                else
                                {
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_015: [ `amqp_management_create` shall create a receiver link by calling `link_create`. ]*/
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_016: [ The `session` argument shall be set to `session`. ]*/
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_017: [ The `name` argument shall be constructed by concatenating the `management_node` value with `-receiver`. ]*/
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_018: [ The `role` argument shall be `role_receiver`. ]*/
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_019: [ The `source` argument shall be the value created by calling `messaging_create_source`. ]*/
                                    /* Codes_SRS_AMQP_MANAGEMENT_01_020: [ The `target` argument shall be the value created by calling `messaging_create_target`. ]*/
                                    result->receiver_link = link_create(session, receiver_link_name, role_receiver, source, target);
                                    if (result->receiver_link == NULL)
                                    {
                                        /* Codes_SRS_AMQP_MANAGEMENT_01_021: [ If `link_create` fails when creating the receiver link then `amqp_management_create` shall fail and return NULL. ]*/
                                        LogError("Failed creating receiver link");
                                        link_destroy(result->sender_link);
                                        free(result);
                                        result = NULL;
                                    }
                                    else
                                    {
                                        /* Codes_SRS_AMQP_MANAGEMENT_01_022: [ `amqp_management_create` shall create a message sender by calling `messagesender_create` and passing to it the sender link handle. ]*/
                                        result->message_sender = messagesender_create(result->sender_link, on_message_sender_state_changed, result);
                                        if (result->message_sender == NULL)
                                        {
                                            /* Codes_SRS_AMQP_MANAGEMENT_01_031: [ If `messagesender_create` fails then `amqp_management_create` shall fail and return NULL. ]*/
                                            LogError("Failed creating message sender");
                                            link_destroy(result->sender_link);
                                            link_destroy(result->receiver_link);
                                            free(result);
                                            result = NULL;
                                        }
                                        else
                                        {
                                            /* Codes_SRS_AMQP_MANAGEMENT_01_023: [ `amqp_management_create` shall create a message receiver by calling `messagereceiver_create` and passing to it the receiver link handle. ]*/
                                            result->message_receiver = messagereceiver_create(result->receiver_link, on_message_receiver_state_changed, result);
                                            if (result->message_receiver == NULL)
                                            {
                                                /* Codes_SRS_AMQP_MANAGEMENT_01_032: [ If `messagereceiver_create` fails then `amqp_management_create` shall fail and return NULL. ]*/
                                                LogError("Failed creating message receiver");
                                                messagesender_destroy(result->message_sender);
                                                link_destroy(result->sender_link);
                                                link_destroy(result->receiver_link);
                                                free(result);
                                                result = NULL;
                                            }
                                            else
                                            {
                                                result->next_message_id = 0;
                                            }
                                        }
                                    }
                                }

                                free(receiver_link_name);
                            }

                            free(sender_link_name);
                        }

                        amqpvalue_destroy(target);
                    }

                    amqpvalue_destroy(source);
                }
            }
        }
    }

    return result;
}

void amqp_management_destroy(AMQP_MANAGEMENT_HANDLE amqp_management)
{
    if (amqp_management == NULL)
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_025: [ If `amqp_management` is NULL, `amqp_management_destroy` shall do nothing. ]*/
        LogError("NULL amqp_management");
    }
    else
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_024: [ `amqp_management_destroy` shall free all the resources allocated by `amqp_management_create`. ]*/
        if (amqp_management->amqp_management_state != AMQP_MANAGEMENT_STATE_IDLE)
        {
            (void)amqp_management_close(amqp_management);
        }

        if (amqp_management->operation_message_count > 0)
        {
            size_t i;
            for (i = 0; i < amqp_management->operation_message_count; i++)
            {
                message_destroy(amqp_management->operation_messages[i]->message);
                free(amqp_management->operation_messages[i]);
            }

            free(amqp_management->operation_messages);
        }

        /* Codes_SRS_AMQP_MANAGEMENT_01_028: [ `amqp_management_destroy` shall free the message sender by calling `messagesender_destroy`. ]*/
        messagesender_destroy(amqp_management->message_sender);
        /* Codes_SRS_AMQP_MANAGEMENT_01_029: [ `amqp_management_destroy` shall free the message receiver by calling `messagereceiver_destroy`. ]*/
        messagereceiver_destroy(amqp_management->message_receiver);
        /* Codes_SRS_AMQP_MANAGEMENT_01_027: [ `amqp_management_destroy` shall free the sender and receiver links by calling `link_destroy`. ]*/
        link_destroy(amqp_management->sender_link);
        link_destroy(amqp_management->receiver_link);
        /* Codes_SRS_AMQP_MANAGEMENT_01_026: [ `amqp_management_destroy` shall free the singly linked list by calling `singlylinkedlist_destroy`. ]*/
        singlylinkedlist_destroy(amqp_management->pending_operations);
        free(amqp_management);
    }
}

int amqp_management_open_async(AMQP_MANAGEMENT_HANDLE amqp_management, ON_AMQP_MANAGEMENT_OPEN_COMPLETE on_amqp_management_open_complete, void* on_amqp_management_open_complete_context, ON_AMQP_MANAGEMENT_ERROR on_amqp_management_error, void* on_amqp_management_error_context)
{
    int result;

    /* Codes_SRS_AMQP_MANAGEMENT_01_044: [ `on_amqp_management_open_complete_context` and `on_amqp_management_error_context` shall be allowed to be NULL. ]*/
    if ((amqp_management == NULL) ||
        (on_amqp_management_open_complete == NULL) ||
        (on_amqp_management_error == NULL))
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_038: [ If `amqp_management`, `on_amqp_management_open_complete` or `on_amqp_management_error` is NULL, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
        LogError("Bad arguments: amqp_management = %p, on_amqp_management_open_complete = %p, on_amqp_management_error = %p",
            amqp_management,
            on_amqp_management_open_complete,
            on_amqp_management_error);
        result = __FAILURE__;
    }
    else if (amqp_management->amqp_management_state != AMQP_MANAGEMENT_STATE_IDLE)
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_043: [ If the AMQP management instance is already OPEN or OPENING, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
        LogError("AMQP management instance already OPEN");
        result = __FAILURE__;
    }
    else
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_036: [ `amqp_management_open_async` shall start opening the AMQP management instance and save the callbacks so that they can be called when opening is complete. ]*/
        amqp_management->on_amqp_management_open_complete = on_amqp_management_open_complete;
        amqp_management->on_amqp_management_open_complete_context = on_amqp_management_open_complete_context;
        amqp_management->on_amqp_management_error = on_amqp_management_error;
        amqp_management->on_amqp_management_error_context = on_amqp_management_error_context;
        amqp_management->amqp_management_state = AMQP_MANAGEMENT_STATE_OPENING;

        /* Codes_SRS_AMQP_MANAGEMENT_01_040: [ `amqp_management_open_async` shall open the message receiver by calling `messagereceiver_open`. ]*/
        if (messagereceiver_open(amqp_management->message_receiver, on_message_received, amqp_management) != 0)
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_042: [ If `messagereceiver_open` fails, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
            LogError("Failed opening message receiver");
            amqp_management->amqp_management_state = AMQP_MANAGEMENT_STATE_IDLE;
            result = __FAILURE__;
        }
        else
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_039: [ `amqp_management_open_async` shall open the message sender by calling `messagesender_open`. ]*/
            if (messagesender_open(amqp_management->message_sender) != 0)
            {
                /* Codes_SRS_AMQP_MANAGEMENT_01_041: [ If `messagesender_open` fails, `amqp_management_open_async` shall fail and return a non-zero value. ]*/
                LogError("Failed opening message sender");
                amqp_management->amqp_management_state = AMQP_MANAGEMENT_STATE_IDLE;
                (void)messagereceiver_close(amqp_management->message_receiver);
                result = __FAILURE__;
            }
            else
            {
                /* Codes_SRS_AMQP_MANAGEMENT_01_037: [ On success it shall return 0. ]*/
                result = 0;
            }
        }
    }

    return result;
}

int amqp_management_close(AMQP_MANAGEMENT_HANDLE amqp_management)
{
    int result;

    if (amqp_management == NULL)
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_047: [ If `amqp_management` is NULL, `amqp_management_close` shall fail and return a non-zero value. ]*/
        LogError("NULL amqp_management");
        result = __FAILURE__;
    }
    else
    {
        /* Codes_SRS_AMQP_MANAGEMENT_01_045: [ `amqp_management_close` shall close the AMQP management instance. ]*/
        /* Codes_SRS_AMQP_MANAGEMENT_01_050: [ `amqp_management_close` shall close the message sender by calling `messagesender_close`. ]*/
        if (messagesender_close(amqp_management->message_sender) != 0)
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_052: [ If `messagesender_close` fails, `amqp_management_close` shall fail and return a non-zero value. ]*/
            LogError("messagesender_close failed");
            result = __FAILURE__;
        }
        /* Codes_SRS_AMQP_MANAGEMENT_01_051: [ `amqp_management_close` shall close the message receiver by calling `messagereceiver_close`. ]*/
        else if (messagereceiver_close(amqp_management->message_receiver) != 0)
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_053: [ If `messagereceiver_close` fails, `amqp_management_close` shall fail and return a non-zero value. ]*/
            LogError("messagereceiver_close failed");
            result = __FAILURE__;
        }
        else
        {
            amqp_management->amqp_management_state = AMQP_MANAGEMENT_STATE_IDLE;

            /* Codes_SRS_AMQP_MANAGEMENT_01_046: [ On success it shall return 0. ]*/
            result = 0;
        }
    }

    return result;
}

int amqp_management_execute_operation_async(AMQP_MANAGEMENT_HANDLE amqp_management, const char* operation, const char* type, const char* locales, MESSAGE_HANDLE message, ON_AMQP_MANAGEMENT_EXECUTE_OPERATION_COMPLETE on_execute_operation_complete, void* on_execute_operation_complete_context)
{
    int result;

    if ((amqp_management == NULL) ||
        (operation == NULL))
    {
        result = __FAILURE__;
    }
    else
    {
        AMQP_VALUE application_properties;

        /* Codes_SRS_AMQP_MANAGEMENT_01_055: [ `amqp_management_execute_operation_async` shall start an AMQP management operation. ]*/
        /* Codes_SRS_AMQP_MANAGEMENT_01_082: [ `amqp_management_execute_operation_async` shall obtain the application properties from the message by calling `message_get_application_properties`. ]*/
        if (message_get_application_properties(message, &application_properties) != 0)
        {
            result = __FAILURE__;
        }
        else
        {
            /* Codes_SRS_AMQP_MANAGEMENT_01_084: [ For each of the arguments `operation`, `type` and `locales` an AMQP value of type string shall be created by calling `amqpvalue_create_string` in order to be used as key in the application properties map. ]*/
            /* Codes_SRS_AMQP_MANAGEMENT_01_085: [ For each of the arguments `operation`, `type` and `locales` an AMQP value of type string containing the argument value shall be created by calling `amqpvalue_create_string` in order to be used as value in the application properties map. ]*/
            if ((add_string_key_value_pair_to_map(application_properties, "operation", operation) != 0) ||
                (add_string_key_value_pair_to_map(application_properties, "type", type) != 0) ||
                ((locales != NULL) && (add_string_key_value_pair_to_map(application_properties, "locales", locales) != 0)))
            {
                result = __FAILURE__;
            }
            else
            {
                /* Codes_SRS_AMQP_MANAGEMENT_01_087: [ The application properties obtained after adding the key/value pairs shall be set on the message by calling `message_set_application_properties`. ]*/
                if ((message_set_application_properties(message, application_properties) != 0) ||
                    (set_message_id(message, amqp_management->next_message_id) != 0))
                {
                    result = __FAILURE__;
                }
                else
                {
                    /* Codes_SRS_AMQP_MANAGEMENT_01_091: [ Once the request message has been sent, an entry shall be stored in the pending operations list by calling `singlylinkedlist_add`. ]*/
                    OPERATION_MESSAGE_INSTANCE* pending_operation_message = malloc(sizeof(OPERATION_MESSAGE_INSTANCE));
                    if (pending_operation_message == NULL)
                    {
                        result = __FAILURE__;
                    }
                    else
                    {
                        pending_operation_message->message = message_clone(message);
                        pending_operation_message->callback_context = on_execute_operation_complete_context;
                        pending_operation_message->on_execute_operation_complete = on_execute_operation_complete;
                        pending_operation_message->operation_state = OPERATION_STATE_NOT_SENT;
                        pending_operation_message->message_id = amqp_management->next_message_id;

                        amqp_management->next_message_id++;

                        /* Codes_SRS_AMQP_MANAGEMENT_01_091: [ Once the request message has been sent, an entry shall be stored in the pending operations list by calling `singlylinkedlist_add`. ]*/
                        if (singlylinkedlist_add(amqp_management->pending_operations, pending_operation_message) != 0)
                        {
                            message_destroy(message);
                            free(pending_operation_message);
                            result = __FAILURE__;
                        }
                        else
                        {
                            /* Codes_SRS_AMQP_MANAGEMENT_01_088: [ `amqp_management_execute_operation_async` shall send the message by calling `messagesender_send`. ]*/
                            if (send_operation_messages(amqp_management) != 0)
                            {
                                if (on_execute_operation_complete != NULL)
                                {
                                    on_execute_operation_complete(on_execute_operation_complete_context, AMQP_MANAGEMENT_EXECUTE_OPERATION_ERROR, 0, NULL);
                                }

                                result = __FAILURE__;
                            }
                            else
                            {
                                /* Codes_SRS_AMQP_MANAGEMENT_01_056: [ On success it shall return 0. ]*/
                                result = 0;
                            }
                        }
                    }
                }
            }

            amqpvalue_destroy(application_properties);
        }
    }
    return result;
}

void amqp_management_set_trace(AMQP_MANAGEMENT_HANDLE amqp_management, bool trace_on)
{
    if (amqp_management != NULL)
    {
        messagesender_set_trace(amqp_management->message_sender, trace_on);
    }
}
