// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/socket_listener.h"
#include "azure_uamqp_c/header_detect_io.h"
#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/tls_server_io.h"

static unsigned int sent_messages = 0;
static const size_t msg_count = 1;
static CONNECTION_HANDLE connection;
static SESSION_HANDLE session;
static LINK_HANDLE link;
static MESSAGE_RECEIVER_HANDLE message_receiver;
static size_t count_received;
static unsigned char* cert_buffer;
static size_t cert_size;

static const char test_certificate[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIC+TCCAeWgAwIBAgIQKuCgUM+ix69PC8DOT9MLhjAJBgUrDgMCHQUAMBYxFDAS\r\n"
"BgNVBAMTC3VBTVFQVGVzdENBMB4XDTE3MDMyODAzNDYyNFoXDTM5MTIzMTIzNTk1\r\n"
"OVowFjEUMBIGA1UEAxMLdUFNUVBUZXN0Q0EwggEiMA0GCSqGSIb3DQEBAQUAA4IB\r\n"
"DwAwggEKAoIBAQCv50a7b/jqYLkT2K+0tCSH0kP3+GW2w7ju4lNvUvPfQZii4dVe\r\n"
"UN63Ntc0CWFAV4+cwVvA43PCVcBejxL77ezcD2EbhkTEiE7C79oQ7vH1UdBUdB3s\r\n"
"pKphNMrUCBPc7dWndyUonoiBVO44Py7iBOKtHss5sWJksxdtzDDY5T7Gmmz5ktjO\r\n"
"UHKIf6R+6v92+IV+DaDdSrZOzVMkBs5KyNMv6BX0+451hlWF0jjI/SFDonqq5Zs9\r\n"
"Wk+sv0uTrTIfzgkRpA1vJESPD6jII+Y28BMDUtqmktxBkEsghxAH7rUASAlfgvAD\r\n"
"h27riy/+s0qOpZ7OrpnhZYUJH1kh+7/U2trlAgMBAAGjSzBJMEcGA1UdAQRAMD6A\r\n"
"EE3hf1snRXbBI5XQMLZdWAShGDAWMRQwEgYDVQQDEwt1QU1RUFRlc3RDQYIQKuCg\r\n"
"UM+ix69PC8DOT9MLhjAJBgUrDgMCHQUAA4IBAQApnqd0fBn82eDq3K+AzpU/Q1y+\r\n"
"0XXeq7ulCM7xC62GdHx4SBMDAk/PUrxrzIFew+D/GXtqFASvPilnMKTTo3Wc4HG8\r\n"
"XYgdGeWCH6k56ru5uYo9jbrTB9FsBhkYzEKBJv1IozQptW4YgUF9G/5VDYazAOel\r\n"
"VSrraThUSjmuHN6HxLbuu/juEJflK2n1K1twi+c2wUZUQsLaXaNxSHpJgiyDReqZ\r\n"
"gQD0cLcnisRNYPEbvF0q18f4JB1yTJimv8/NBc1x6H+qEW5TWv7SYE8sCJh/76ns\r\n"
"klRqV4HSHFxO8z9MX/c/XkU5i+Q9tRmRw5l8shGRS5lvlma/HpE1ucfbIfTA\r\n"
"-----END CERTIFICATE-----\r\n";

static const char test_private_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEowIBAAKCAQEAr+dGu2/46mC5E9ivtLQkh9JD9/hltsO47uJTb1Lz30GYouHV\r\n"
"XlDetzbXNAlhQFePnMFbwONzwlXAXo8S++3s3A9hG4ZExIhOwu/aEO7x9VHQVHQd\r\n"
"7KSqYTTK1AgT3O3Vp3clKJ6IgVTuOD8u4gTirR7LObFiZLMXbcww2OU+xpps+ZLY\r\n"
"zlByiH+kfur/dviFfg2g3Uq2Ts1TJAbOSsjTL+gV9PuOdYZVhdI4yP0hQ6J6quWb\r\n"
"PVpPrL9Lk60yH84JEaQNbyREjw+oyCPmNvATA1LappLcQZBLIIcQB+61AEgJX4Lw\r\n"
"A4du64sv/rNKjqWezq6Z4WWFCR9ZIfu/1Nra5QIDAQABAoIBAA2QzYPcQWfgTuwS\r\n"
"TikM3ebBNxxJjzaTTSptg+rvv8to8kElheBnMwKrYdj4HDe4SP5y0jh2+IRCCFSC\r\n"
"iv+o1eQTjFtxQgTSHX9FIuq8mSZgYxIQSzqAnHv9dHyTpUyCS9oSNmho7+gUr7NH\r\n"
"Lb6g8Wy4FXFjYufKZzUnH5R3YYPMkxZp/KkkVMCB4Jj8yPVKzR1ntw5b6YO4obm6\r\n"
"a5P2Yi+0g84AOLjsHSU6cLf7oeH5/8xeYzEubIWX1QcUd5dhMOp2+9OycKqfttrh\r\n"
"s8rP085+ivQeYYws+k125T2Esd6/ARmhclLcFKK/UMJD9Q7mtWCKbzQf72eWu26U\r\n"
"7O4py2UCgYEA12/cXfFyGi5ncDPfIo1YwCQ9Fwoaic0xY0zXomnug8n4yNCMGODw\r\n"
"JAzoDhC7AgPwrbX7MWdidbokVu1YgDCQu4Gcm86u/+S3xQd+i8cCA7UQva9OksqO\r\n"
"48hkGWig0n3HOfnKS1RVlpZtKgZEJmREUqEkOVlnvVJmeXq+3s1YHicCgYEA0QXi\r\n"
"iEdbhOR2GH4dDn73JLdTKf/vURB/h4rLlhWIHcvZdxiALZFX29hCwhBIL80T0lPL\r\n"
"tfc8IjxgK0viOv5dXs0mS7ctO/sH/74Qm6O0ToB0tgURP2r8F2zSkJxPGdvrJ48B\r\n"
"Jbo57JI5nNHu/PVw/tI/JzJIbEW7tYQbxJRLMhMCgYEAx5RIWPs5UknU55wWRaMe\r\n"
"KfooYfSpOynNbAme5kYugQaVpCuW7eFMdolCXO1g4XAXAkZJa640B44m5iTAzRiw\r\n"
"rBRZqfmiI0uWd0AHGqSFGDwgQylpqBFgqGJXYTaNbhK5gtsGbhy1oWi/vqPJdKuE\r\n"
"o+vGbB6IPVpdtoJg2nTvAhcCgYBan5CNwVJelabWC6eRZ17DnnACH6KkpOCF5ZlK\r\n"
"4t72/DC2v/qixwcum96lwOVrRCC56fbCWATMWxze6LGXHj1hItTdsvd7r+TR7pfI\r\n"
"wvsjpfH0ENJfioTtqxLH+90Xuw+DQS8gKlN+zA8KfMJ/DfMFqCYVWmmn97vggPyB\r\n"
"CEJp3QKBgCGWSSFtWCbuk3Q5FSiLSEjL/NZ0jSMQZnSp43lXNW7fm6fl0ZVVog6E\r\n"
"/YZWK4ckqXZtcIVfbiI4Vjt06Pnj+v3it+41DM6xQejq6fFZZciQUdXyrGfWLm6P\r\n"
"ARVSw5sYh+cfit382DDQ+LAEbveNy9Hhq36m3b/7DHUsYwqactuT\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

static void on_message_receiver_state_changed(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    (void)context, new_state, previous_state;
}

static AMQP_VALUE on_message_received(const void* context, MESSAGE_HANDLE message)
{
	(void)context;
	(void)message;

    if ((count_received % 1000) == 0)
    {
        printf("Messages received : %u.\r\n", (unsigned int)count_received);
    }
    count_received++;

	return messaging_delivery_accepted();
}

static bool on_new_link_attached(void* context, LINK_ENDPOINT_HANDLE new_link_endpoint, const char* name, role role, AMQP_VALUE source, AMQP_VALUE target)
{
    (void)context;
	link = link_create_from_endpoint(session, new_link_endpoint, name, role, source, target);
	link_set_rcv_settle_mode(link, receiver_settle_mode_first);
	message_receiver = messagereceiver_create(link, on_message_receiver_state_changed, NULL);
	messagereceiver_open(message_receiver, on_message_received, NULL);
	return true;
}

static bool on_new_session_endpoint(void* context, ENDPOINT_HANDLE new_endpoint)
{
    (void)context;
	session = session_create_from_endpoint(connection, new_endpoint, on_new_link_attached, NULL);
	session_set_incoming_window(session, 10000);
	session_begin(session);
	return true;
}

static void on_socket_accepted(void* context, const IO_INTERFACE_DESCRIPTION* interface_description, void* io_parameters)
{
	HEADERDETECTIO_CONFIG header_detect_io_config;
    TLS_SERVER_IO_CONFIG tls_server_io_config;
    XIO_HANDLE underlying_io;

    (void)context;

    tls_server_io_config.certificate = cert_buffer;
    tls_server_io_config.certificate_size = cert_size;
    tls_server_io_config.underlying_io_interface = interface_description;
    tls_server_io_config.underlying_io_parameters = io_parameters;

    underlying_io = xio_create(tls_server_io_get_interface_description(), &tls_server_io_config);

//    xio_setoption(underlying_io, "x509certificate", test_certificate);
  //  xio_setoption(underlying_io, "x509privatekey", test_private_key);

    header_detect_io_config.underlying_io = underlying_io;
	XIO_HANDLE header_detect_io = xio_create(headerdetectio_get_interface_description(), &header_detect_io_config);
	connection = connection_create(header_detect_io, NULL, "1", on_new_session_endpoint, NULL);
	connection_listen(connection);
}

int main(int argc, char** argv)
{
	int result;

    (void)argc, argv;

	if (platform_init() != 0)
	{
		result = -1;
	}
	else
	{
		size_t last_memory_used = 0;
        FILE* cert_file;

        gballoc_init();

        cert_file = fopen("uamqptestca.cer", "rb");
        if (cert_file == NULL)
        {
            result = -1;
        }
        else
        {
            fseek(cert_file, 0, SEEK_END);
            cert_size = ftell(cert_file);

            cert_buffer = malloc(cert_size);
            if (cert_buffer == NULL)
            {
                result = -1;
            }
            else
            {
                fseek(cert_file, 0, SEEK_SET);
                fread(cert_buffer, 1, cert_size, cert_file);

                SOCKET_LISTENER_HANDLE socket_listener = socketlistener_create(5671);
                if (socketlistener_start(socket_listener, on_socket_accepted, NULL) != 0)
                {
                    result = -1;
                }
                else
                {
                    while (true)
                    {
                        size_t current_memory_used;
                        size_t maximum_memory_used;
                        socketlistener_dowork(socket_listener);

                        current_memory_used = gballoc_getCurrentMemoryUsed();
                        maximum_memory_used = gballoc_getMaximumMemoryUsed();

                        if (current_memory_used != last_memory_used)
                        {
                            printf("Current memory usage:%lu (max:%lu)\r\n", (unsigned long)current_memory_used, (unsigned long)maximum_memory_used);
                            last_memory_used = current_memory_used;
                        }

                        if (sent_messages == msg_count)
                        {
                            break;
                        }

                        if (connection != NULL)
                        {
                            connection_dowork(connection);
                        }
                    }

                    result = 0;
                }

                socketlistener_destroy(socket_listener);
                platform_deinit();

                printf("Max memory usage:%lu\r\n", (unsigned long)gballoc_getCurrentMemoryUsed());
                printf("Current memory usage:%lu\r\n", (unsigned long)gballoc_getMaximumMemoryUsed());

                gballoc_deinit();

                free(cert_buffer);
            }

            fclose(cert_file);
        }
    }

	return result;
}
