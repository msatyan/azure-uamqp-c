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

**SRS_AMQP_MANAGEMENT_01_055: [** `amqp_management_execute_operation_async` shall start an AMQP management operation. **]**
**SRS_AMQP_MANAGEMENT_01_056: [** On success it shall return 0. **]**
**SRS_AMQP_MANAGEMENT_01_057: [** If `amqp_management`, `operation`, `type`, `message` or `on_execute_operation_complete` is NULL, `amqp_management_execute_operation_async` shall fail and return a non-zero value. **]**

### amqp_management_set_trace

```c
void amqp_management_set_trace(AMQP_MANAGEMENT_HANDLE amqp_management, bool trace_on);
```

