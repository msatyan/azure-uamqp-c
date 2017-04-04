# amqp_management requirements

## Overview

`amqp_management` is module that implements the AMQP management draft specification portion that refers to operations (request/response pattern).

##Exposed API

```c
    typedef enum AMQP_MANAGEMENT_EXECUTE_OPERATION_RESULT_TAG
    {
        AMQP_MANAGEMENT_EXECUTE_OPERATION_OK,
        AMQP_MANAGEMENT_EXECUTE_OPERATION_ERROR,
        AMQP_MANAGEMENT_EXECUTE_OPERATION_FAILED_BAD_STATUS,
        AMQP_MANAGEMENT_EXECUTE_OPERATION_INSTANCE_CLOSED
    } AMQP_MANAGEMENT_EXECUTE_OPERATION_RESULT;

    typedef enum AMQP_MANAGEMENT_OPEN_RESULT_TAG
    {
        AMQP_MANAGEMENT_OPEN_OK,
        AMQP_MANAGEMENT_OPEN_ERROR,
        AMQP_MANAGEMENT_OPEN_CANCELLED
    } AMQP_MANAGEMENT_OPEN_RESULT;

    typedef struct AMQP_MANAGEMENT_INSTANCE_TAG* AMQP_MANAGEMENT_HANDLE;
    typedef void(*ON_AMQP_MANAGEMENT_OPEN_COMPLETE)(void* context, AMQP_MANAGEMENT_OPEN_RESULT open_result);
    typedef void(*ON_AMQP_MANAGEMENT_ERROR)(void* context);
    typedef void(*ON_AMQP_MANAGEMENT_EXECUTE_OPERATION_COMPLETE)(void* context, AMQP_MANAGEMENT_EXECUTE_OPERATION_RESULT execute_operation_result, unsigned int status_code, const char* status_description);

    MOCKABLE_FUNCTION(, AMQP_MANAGEMENT_HANDLE, amqp_management_create, SESSION_HANDLE, session, const char*, management_node);
    MOCKABLE_FUNCTION(, void, amqp_management_destroy, AMQP_MANAGEMENT_HANDLE, amqp_management);
    MOCKABLE_FUNCTION(, int, amqp_management_open_async, AMQP_MANAGEMENT_HANDLE, amqp_management, ON_AMQP_MANAGEMENT_OPEN_COMPLETE, on_amqp_management_open_complete, void*, on_amqp_management_open_complete_context, ON_AMQP_MANAGEMENT_ERROR, on_amqp_management_error, void*, on_amqp_management_error_context);
    MOCKABLE_FUNCTION(, int, amqp_management_close, AMQP_MANAGEMENT_HANDLE, amqp_management);
    MOCKABLE_FUNCTION(, int, amqp_management_execute_operation_async, AMQP_MANAGEMENT_HANDLE, amqp_management, const char*, operation, const char*, type, const char*, locales, MESSAGE_HANDLE, message, ON_AMQP_MANAGEMENT_EXECUTE_OPERATION_COMPLETE, on_execute_operation_complete, void*, context);
    MOCKABLE_FUNCTION(, void, amqp_management_set_trace, AMQP_MANAGEMENT_HANDLE, amqp_management, bool, trace_on);
```

### amqp_management_create

```c
AMQP_MANAGEMENT_HANDLE amqp_management_create(SESSION_HANDLE session, const char* management_node);
```

XX**SRS_AMQP_MANAGEMENT_01_001: [** `amqp_management_create` shall create a new CBS instance and on success return a non-NULL handle to it. **]**
XX**SRS_AMQP_MANAGEMENT_01_002: [** If `session` or `management_node` is NULL then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_030: [** If `management_node` is an empty string, then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_003: [** `amqp_management_create` shall create a singly linked list for pending operations by calling `singlylinkedlist_create`. **]**
XX**SRS_AMQP_MANAGEMENT_01_004: [** If `singlylinkedlist_create` fails, `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_005: [** If allocating memory for the new handle fails, `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_006: [** `amqp_management_create` shall create a sender link by calling `link_create`. **]**
XX**SRS_AMQP_MANAGEMENT_01_007: [** The `session` argument shall be set to `session`. **]**
XX**SRS_AMQP_MANAGEMENT_01_008: [** The `name` argument shall be constructed by concatenating the `management_node` value with `-sender`. **]**
XX**SRS_AMQP_MANAGEMENT_01_009: [** The `role` argument shall be `role_sender`. **]**
XX**SRS_AMQP_MANAGEMENT_01_010: [** The `source` argument shall be a value created by calling `messaging_create_source` with `management_node` as argument. **]**
XX**SRS_AMQP_MANAGEMENT_01_011: [** The `target` argument shall be a value created by calling `messaging_create_target` with `management_node` as argument. **]**
XX**SRS_AMQP_MANAGEMENT_01_012: [** If `messaging_create_source` fails then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_013: [** If `messaging_create_target` fails then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_014: [** If `link_create` fails when creating the sender link then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_015: [** `amqp_management_create` shall create a receiver link by calling `link_create`. **]**
XX**SRS_AMQP_MANAGEMENT_01_016: [** The `session` argument shall be set to `session`. **]**
XX**SRS_AMQP_MANAGEMENT_01_017: [** The `name` argument shall be constructed by concatenating the `management_node` value with `-receiver`. **]**
XX**SRS_AMQP_MANAGEMENT_01_018: [** The `role` argument shall be `role_receiver`. **]**
XX**SRS_AMQP_MANAGEMENT_01_019: [** The `source` argument shall be the value created by calling `messaging_create_source`. **]**
XX**SRS_AMQP_MANAGEMENT_01_020: [** The `target` argument shall be the value created by calling `messaging_create_target`. **]**
XX**SRS_AMQP_MANAGEMENT_01_021: [** If `link_create` fails when creating the receiver link then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_022: [** `amqp_management_create` shall create a message sender by calling `messagesender_create` and passing to it the sender link handle. **]**
XX**SRS_AMQP_MANAGEMENT_01_023: [** `amqp_management_create` shall create a message receiver by calling `messagereceiver_create` and passing to it the receiver link handle. **]**
XX**SRS_AMQP_MANAGEMENT_01_031: [** If `messagesender_create` fails then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_032: [** If `messagereceiver_create` fails then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_033: [** If any other error occurs `amqp_management_create` shall fail and return NULL. **]**

### amqp_management_destroy

```c
void amqp_management_destroy(AMQP_MANAGEMENT_HANDLE amqp_management);
```

XX**SRS_AMQP_MANAGEMENT_01_024: [** `amqp_management_destroy` shall free all the resources allocated by `amqp_management_create`. **]**
XX**SRS_AMQP_MANAGEMENT_01_025: [** If `amqp_management` is NULL, `amqp_management_destroy` shall do nothing. **]**
XX**SRS_AMQP_MANAGEMENT_01_026: [** `amqp_management_destroy` shall free the singly linked list by calling `singlylinkedlist_destroy`. **]**
XX**SRS_AMQP_MANAGEMENT_01_027: [** `amqp_management_destroy` shall free the sender and receiver links by calling `link_destroy`. **]**
XX**SRS_AMQP_MANAGEMENT_01_028: [** `amqp_management_destroy` shall free the message sender by calling `messagesender_destroy`. **]**
XX**SRS_AMQP_MANAGEMENT_01_029: [** `amqp_management_destroy` shall free the message receiver by calling `messagereceiver_destroy`. **]**
**SRS_AMQP_MANAGEMENT_01_034: [** If the AMQP management instance is OPEN or OPENING, `amqp_management_destroy` shall also perform all actions that would be done by `amqp_management_close`. **]**

### amqp_management_open_async

```c
int amqp_management_open_async(AMQP_MANAGEMENT_HANDLE amqp_management, ON_AMQP_MANAGEMENT_OPEN_COMPLETE on_amqp_management_open_complete, void* on_amqp_management_open_complete_context, ON_AMQP_MANAGEMENT_ERROR on_amqp_management_error, void* on_amqp_management_error_context);
```

XX**SRS_AMQP_MANAGEMENT_01_036: [** `amqp_management_open_async` shall start opening the AMQP management instance and save the callbacks so that they can be called when opening is complete. **]**
XX**SRS_AMQP_MANAGEMENT_01_037: [** On success it shall return 0. **]**
XX**SRS_AMQP_MANAGEMENT_01_038: [** If `amqp_management`, `on_amqp_management_open_complete` or `on_amqp_management_error` is NULL, `amqp_management_open_async` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_044: [** `on_amqp_management_open_complete_context` and `on_amqp_management_error_context` shall be allowed to be NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_039: [** `amqp_management_open_async` shall open the message sender by calling `messagesender_open`. **]**
XX**SRS_AMQP_MANAGEMENT_01_040: [** `amqp_management_open_async` shall open the message receiver by calling `messagereceiver_open`. **]**
XX**SRS_AMQP_MANAGEMENT_01_041: [** If `messagesender_open` fails, `amqp_management_open_async` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_042: [** If `messagereceiver_open` fails, `amqp_management_open_async` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_043: [** If the AMQP management instance is already OPEN or OPENING, `amqp_management_open_async` shall fail and return a non-zero value. **]**

### amqp_management_close

```c
int amqp_management_close(AMQP_MANAGEMENT_HANDLE amqp_management);
```

XX**SRS_AMQP_MANAGEMENT_01_045: [** `amqp_management_close` shall close the AMQP management instance. **]**
XX**SRS_AMQP_MANAGEMENT_01_046: [** On success it shall return 0. **]**
XX**SRS_AMQP_MANAGEMENT_01_047: [** If `amqp_management` is NULL, `amqp_management_close` shall fail and return a non-zero value. **]**
**SRS_AMQP_MANAGEMENT_01_048: [** `amqp_management_close` on an AMQP management instance that is OPENING shall trigger the `on_amqp_management_open_complete` callback with `AMQP_MANAGEMENT_OPEN_CANCELLED`, while also passing the context passed in `amqp_management_open_async`. **]**
**SRS_AMQP_MANAGEMENT_01_049: [** `amqp_management_close` on an AMQP management instance that is not OPEN, shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_050: [** `amqp_management_close` shall close the message sender by calling `messagesender_close`. **]**
XX**SRS_AMQP_MANAGEMENT_01_051: [** `amqp_management_close` shall close the message receiver by calling `messagereceiver_close`. **]**
XX**SRS_AMQP_MANAGEMENT_01_052: [** If `messagesender_close` fails, `amqp_management_close` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_053: [** If `messagereceiver_close` fails, `amqp_management_close` shall fail and return a non-zero value. **]**
**SRS_AMQP_MANAGEMENT_01_054: [** All pending operations shall be indicated complete with the code `AMQP_MANAGEMENT_EXECUTE_OPERATION_INSTANCE_CLOSED`. **]**

### amqp_management_execute_operation_async

```c
int amqp_management_execute_operation_async(AMQP_MANAGEMENT_HANDLE amqp_management, const char* operation, const char* type, const char* locales, MESSAGE_HANDLE message, ON_AMQP_MANAGEMENT_EXECUTE_OPERATION_COMPLETE on_execute_operation_complete, void* context);
```

XX**SRS_AMQP_MANAGEMENT_01_055: [** `amqp_management_execute_operation_async` shall start an AMQP management operation. **]**
XX**SRS_AMQP_MANAGEMENT_01_056: [** On success it shall return 0. **]**
**SRS_AMQP_MANAGEMENT_01_057: [** If `amqp_management`, `operation`, `type`, `message` or `on_execute_operation_complete` is NULL, `amqp_management_execute_operation_async` shall fail and return a non-zero value. **]**
**SRS_AMQP_MANAGEMENT_01_081: [** If `amqp_management_execute_operation_async` is called when not OPEN, it shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_082: [** `amqp_management_execute_operation_async` shall obtain the application properties from the message by calling `message_get_application_properties`. **]**
**SRS_AMQP_MANAGEMENT_01_083: [** If no application properties were set on the message, a new application properties instance shall be created by calling `amqpvalue_create_map`; **]**
XX**SRS_AMQP_MANAGEMENT_01_084: [** For each of the arguments `operation`, `type` and `locales` an AMQP value of type string shall be created by calling `amqpvalue_create_string` in order to be used as key in the application properties map. **]**
XX**SRS_AMQP_MANAGEMENT_01_085: [** For each of the arguments `operation`, `type` and `locales` an AMQP value of type string containing the argument value shall be created by calling `amqpvalue_create_string` in order to be used as value in the application properties map. **]**
**SRS_AMQP_MANAGEMENT_01_093: [** If `locales` NULL, no key/value pair shall be added for it in the application properties map. **]**
XX**SRS_AMQP_MANAGEMENT_01_086: [** The key/value pairs for `operation`, `type` and `locales` shall be added to the application properties map by calling `amqpvalue_set_map_value`. **]**
XX**SRS_AMQP_MANAGEMENT_01_087: [** The application properties obtained after adding the key/value pairs shall be set on the message by calling `message_set_application_properties`. **]**
**SRS_AMQP_MANAGEMENT_01_101: [** After setting the application properties, the application properties instance shall be freed by `amqpvalue_destroy`. **]**
**SRS_AMQP_MANAGEMENT_01_090: [** If any APIs used to create and set the application properties on the message fails, `amqp_management_execute_operation_async` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_094: [** In order to set the message Id on the message, the properties shall be obtained by calling `message_get_properties`. **]**
**SRS_AMQP_MANAGEMENT_01_099: [** If the properties cannot be obtained, a new properties instance shall be created by calling `properties_create`. **]**
XX**SRS_AMQP_MANAGEMENT_01_095: [** A message Id with the next ulong value to be used shall be created by calling `amqpvalue_create_message_id_ulong`. **]**
XX**SRS_AMQP_MANAGEMENT_01_096: [** The message Id value shall be set on the properties by calling `properties_set_message_id`. **]**
XX**SRS_AMQP_MANAGEMENT_01_097: [** The properties thus modified to contain the message Id shall be set on the message by calling `message_set_properties`. **]**
XX**SRS_AMQP_MANAGEMENT_01_100: [** After setting the properties, the properties instance shall be freed by `properties_destroy`. **]**
**SRS_AMQP_MANAGEMENT_01_098: [** If any API fails while setting the message Id, `amqp_management_execute_operation_async` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_088: [** `amqp_management_execute_operation_async` shall send the message by calling `messagesender_send`. **]**
**SRS_AMQP_MANAGEMENT_01_089: [** If `messagesender_send` fails, `amqp_management_execute_operation_async` shall fail and return a non-zero value. **]**
XX**SRS_AMQP_MANAGEMENT_01_091: [** Once the request message has been sent, an entry shall be stored in the pending operations list by calling `singlylinkedlist_add`. **]**
**SRS_AMQP_MANAGEMENT_01_092: [** If `singlylinkedlist_add` fails then `amqp_management_execute_operation_async` shall fail and return a non-zero value. **]**

### amqp_management_set_trace

```c
void amqp_management_set_trace(AMQP_MANAGEMENT_HANDLE amqp_management, bool trace_on);
```

### Relevant sections from the AMQP Management spec

Request Messages

**SRS_AMQP_MANAGEMENT_01_058: [** Request messages have the following application-properties: **]**

Key Value Type Mandatory? Description

**SRS_AMQP_MANAGEMENT_01_059: [** operation string Yes The management operation to be performed. **]** This is case-sensitive.

**SRS_AMQP_MANAGEMENT_01_061: [** type string Yes The Manageable Entity Type of the Manageable Entity to be managed. **]** This is case-sensitive.

**SRS_AMQP_MANAGEMENT_01_063: [** locales string No A list of locales that the sending peer permits for incoming informational text in response messages. **]**
**SRS_AMQP_MANAGEMENT_01_064: [** The value MUST be of the form (presented in the augmented BNF defined in section 2 of [RFC2616]) **]**:
**SRS_AMQP_MANAGEMENT_01_065: [** `#Language-­Tag` where Language-Tag is defined in [BCP47] **]**.
**SRS_AMQP_MANAGEMENT_01_066: [** This list MUST be ordered in decreasing level of preference. **]** **SRS_AMQP_MANAGEMENT_01_067: [** The receiving partner will choose the first (most preferred) incoming locale from those which it supports. **]** If none of the requested locales are supported, "en-US" MUST be chosen. Note that "en-US" need not be supplied in this list as it is always the fallback. The string is not case-sensitive.
Other application-properties MAY provide additional context. If an application-property is not recognized then it MUST be ignored.

3.2 Response Messages

**SRS_AMQP_MANAGEMENT_01_068: [** The correlation-id of the response message MUST be the correlation-id from the request message (if present) **]**, **SRS_AMQP_MANAGEMENT_01_069: [** else the message-id from the request message. **]**
**SRS_AMQP_MANAGEMENT_01_070: [** Response messages have the following application-properties: **]**

Key Value Type Mandatory? Description

**SRS_AMQP_MANAGEMENT_01_071: [** statusCode integer Yes HTTP response code [RFC2616] **]**

**SRS_AMQP_MANAGEMENT_01_072: [** statusDescription string No Description of the status. **]**

**SRS_AMQP_MANAGEMENT_01_073: [** The type and contents of the body are operation-specific. **]**

3.2.1 Successful Operations

**SRS_AMQP_MANAGEMENT_01_074: [** Successful operations MUST result in a statusCode in the 2xx range as defined in Section 10.2 of [RFC2616]. **]**
Further details including the form of the body are provided in the definition of each operation.

3.2.2 Unsuccessful Operations

**SRS_AMQP_MANAGEMENT_01_075: [** Unsuccessful operations MUST NOT result in a statusCode in the 2xx range as defined in Section 10.2 of [RFC2616]. **]**
**SRS_AMQP_MANAGEMENT_01_076: [** The following error status code SHOULD be used for the following common failure scenarios: **]**

statusCode Label Meaning

**SRS_AMQP_MANAGEMENT_01_077: [** 501 Not Implemented The operation is not supported. **]**
**SRS_AMQP_MANAGEMENT_01_078: [** 404 Not Found The Manageable Entity on which to perform the operation could not be found **]**

Further details of operation-specific codes are provided in the definition of each operation.
**SRS_AMQP_MANAGEMENT_01_079: [** The status description of a response to an unsuccessful operation SHOULD provide further information on the nature of the failure. **]**

The form of the body of a response to an unsuccessful operation is unspecified and MAY be implementation-dependent.
**SRS_AMQP_MANAGEMENT_01_080: [** Clients SHOULD ignore the body of response message if the statusCode is not in the 2xx range. **]**