# amqp_management requirements

## Overview

`amqp_management` is module that implements the AMQP management draft specification portion that refers to operations (request/response pattern).

##Exposed API

```c
    typedef enum AMQP_MANAGEMENT_EXECUTE_OPERATION_RESULT_TAG
    {
        AMQP_MANAGEMENT_EXECUTE_OPERATION_OK,
        AMQP_MANAGEMENT_EXECUTE_OPERATION_ERROR,
        AMQP_MANAGEMENT_EXECUTE_OPERATION_FAILED_BAD_STATUS
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
    MOCKABLE_FUNCTION(, void, amqp_management_set_trace, AMQP_MANAGEMENT_HANDLE, amqp_management, bool, traceOn);
```

### amqp_management_create

```c
AMQP_MANAGEMENT_HANDLE amqp_management_create(SESSION_HANDLE session, const char* management_node);
```

XX**SRS_AMQP_MANAGEMENT_01_001: [** `amqp_management_create` shall create a new CBS instance and on success return a non-NULL handle to it. **]**
XX**SRS_AMQP_MANAGEMENT_01_002: [** If `session` or `management_node` is NULL then `amqp_management_create` shall fail and return NULL. **]**
**SRS_AMQP_MANAGEMENT_01_030: [** If `management_node` is an empty string, then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_003: [** `amqp_management_create` shall create a singly linked list for pending operations by calling `singlylinkedlist_create`. **]**
**SRS_AMQP_MANAGEMENT_01_004: [** If `singlylinkedlist_create` fails, `amqp_management_create` shall fail and return NULL. **]**
**SRS_AMQP_MANAGEMENT_01_005: [** If allocating memory for the new handle fails, `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_006: [** `amqp_management_create` shall create a sender link by calling `link_create`. **]**
**SRS_AMQP_MANAGEMENT_01_007: [** The `session` argument shall be set to `session`. **]**
**SRS_AMQP_MANAGEMENT_01_008: [** The `name` argument shall be constructed by concatenating the `management_node` value with `-sender`. **]**
**SRS_AMQP_MANAGEMENT_01_009: [** The `role` argument shall be `role_sender`. **]**
XX**SRS_AMQP_MANAGEMENT_01_010: [** The `source` argument shall be a value created by calling `messaging_create_source` with `management_node` as argument. **]**
XX**SRS_AMQP_MANAGEMENT_01_011: [** The `target` argument shall be a value created by calling `messaging_create_target` with `management_node` as argument. **]**
**SRS_AMQP_MANAGEMENT_01_012: [** If `messaging_create_source` fails then `amqp_management_create` shall fail and return NULL. **]**
**SRS_AMQP_MANAGEMENT_01_013: [** If `messaging_create_target` fails then `amqp_management_create` shall fail and return NULL. **]**
**SRS_AMQP_MANAGEMENT_01_014: [** If `link_create` fails when creating the sender link then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_015: [** `amqp_management_create` shall create a receiver link by calling `link_create`. **]**
**SRS_AMQP_MANAGEMENT_01_016: [** The `session` argument shall be set to `session`. **]**
**SRS_AMQP_MANAGEMENT_01_017: [** The `name` argument shall be constructed by concatenating the `management_node` value with `-receiver`. **]**
**SRS_AMQP_MANAGEMENT_01_018: [** The `role` argument shall be `role_receiver`. **]**
**SRS_AMQP_MANAGEMENT_01_019: [** The `source` argument shall be the value created by calling `messaging_create_source`. **]**
**SRS_AMQP_MANAGEMENT_01_020: [** The `target` argument shall be the value created by calling `messaging_create_target`. **]**
**SRS_AMQP_MANAGEMENT_01_021: [** If `link_create` fails when creating the receiver link then `amqp_management_create` shall fail and return NULL. **]**
XX**SRS_AMQP_MANAGEMENT_01_022: [** `amqp_management_create` shall create a message sender by calling `messagesender_create` and passing to it the sender link handle. **]**
**SRS_AMQP_MANAGEMENT_01_023: [** `amqp_management_create` shall create a message receiver by calling `messagereceiver_create` and passing to it the receiver link handle. **]**
**SRS_AMQP_MANAGEMENT_01_031: [** If `messagesender_create` fails then `amqp_management_create` shall fail and return NULL. **]**
**SRS_AMQP_MANAGEMENT_01_032: [** If `messagereceiver_create` fails then `amqp_management_create` shall fail and return NULL. **]**

### amqp_management_destroy

```c
void amqp_management_destroy(AMQP_MANAGEMENT_HANDLE amqp_management);
```

**SRS_AMQP_MANAGEMENT_01_024: [** `amqp_management_destroy` shall free all the resources allocated by `amqp_management_create`. **]**
**SRS_AMQP_MANAGEMENT_01_025: [** If `amqp_management` is NULL, `amqp_management_destroy` shall do nothing. **]**
**SRS_AMQP_MANAGEMENT_01_026: [** `amqp_management_destroy` shall free the singly linked list by calling `singlylinkedlist_destroy`. **]**
**SRS_AMQP_MANAGEMENT_01_027: [** `amqp_management_destroy` shall free the sender and receiver links by calling `link_destroy`. **]**
**SRS_AMQP_MANAGEMENT_01_028: [** `amqp_management_destroy` shall free the message sender by calling `messagesender_destroy`. **]**
**SRS_AMQP_MANAGEMENT_01_029: [** `amqp_management_destroy` shall free the message receiver by calling `messagereceiver_destroy`. **]**
