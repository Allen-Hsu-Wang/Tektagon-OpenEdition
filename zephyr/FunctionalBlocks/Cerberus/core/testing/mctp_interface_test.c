// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "platform.h"
#include "testing.h"
#include "crypto/checksum.h"
#include "mock/cmd_interface_mock.h"
#include "mctp/mctp_interface.h"
#include "mctp/mctp_protocol.h"
#include "mctp/mctp_interface_control.h"
#include "cmd_interface/cerberus_protocol.h"
#include "cmd_interface/cerberus_protocol_master_commands.h"
#include "cmd_interface/cmd_interface_system.h"


static const char *SUITE = "mctp_interface";


/**
 * Length of the MCTP header.
 */
#define	MCTP_HEADER_LENGTH		7

/**
 * Length of an MCTP error message.
 */
#define	MCTP_ERROR_MSG_LENGTH	(MCTP_HEADER_LENGTH + sizeof (struct cerberus_protocol_error) + 1)


/**
 * Helper function to setup the MCTP interface to use a mock cmd_interface
 *
 * @param test The test framework
 * @param cmd_interface The cmd interface mock instance to initialize
 * @param device_mgr The device manager instance to initialize
 * @param interface The MCTP interface instance to initialize
 */
static void setup_mctp_interface_with_interface_mock_test (CuTest *test,
	struct cmd_interface_mock *cmd_interface, struct device_manager *device_mgr,
	struct mctp_interface *interface)
{
	struct device_manager_full_capabilities capabilities;
	int status;

	status = cmd_interface_mock_init (cmd_interface);
	CuAssertIntEquals (test, 0, status);

	status = device_manager_init (device_mgr, 2, DEVICE_MANAGER_AC_ROT_MODE,
		DEVICE_MANAGER_SLAVE_BUS_ROLE);
	CuAssertIntEquals (test, 0, status);

	status = device_manager_update_device_entry (device_mgr, 0, DEVICE_MANAGER_SELF,
		MCTP_PROTOCOL_PA_ROT_CTRL_EID, 0);
	CuAssertIntEquals (test, 0, status);

	status = device_manager_update_device_entry (device_mgr, 1, DEVICE_MANAGER_UPSTREAM,
		MCTP_PROTOCOL_BMC_EID, 0);
	CuAssertIntEquals (test, 0, status);

	device_manager_get_device_capabilities (device_mgr, 0, &capabilities);
	capabilities.request.hierarchy_role = DEVICE_MANAGER_PA_ROT_MODE;

	status = device_manager_update_device_capabilities (device_mgr, 0, &capabilities);
	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_init (interface, &cmd_interface->base, device_mgr,
		MCTP_PROTOCOL_PA_ROT_CTRL_EID, CERBERUS_PROTOCOL_MSFT_PCI_VID,
		CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	CuAssertIntEquals (test, 0, status);
}

/**
 * Helper function to complete MCTP test
 *
 * @param test The test framework
 * @param cmd_interface The cmd interface mock instance to release
 * @param device_mgr The device manager instance to release
 * @param interface The MCTP interface instance to release
 */
static void complete_mctp_interface_with_interface_mock_test (CuTest *test,
	struct cmd_interface_mock *cmd_interface, struct device_manager *device_mgr,
	struct mctp_interface *interface)
{
	int status;

	status = cmd_interface_mock_validate_and_release (cmd_interface);
	CuAssertIntEquals (test, 0, status);

	device_manager_release (device_mgr);
	mctp_interface_deinit (interface);
}


/*******************
 * Test cases
 *******************/

static void mctp_interface_test_init (CuTest *test)
{
	int status;
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;

	TEST_START;

	status = cmd_interface_mock_init (&cmd_interface);
	CuAssertIntEquals (test, 0, status);

	status = device_manager_init (&device_mgr, 1, DEVICE_MANAGER_AC_ROT_MODE,
		DEVICE_MANAGER_SLAVE_BUS_ROLE);
	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_init (&interface, &cmd_interface.base, &device_mgr,
		MCTP_PROTOCOL_PA_ROT_CTRL_EID, CERBERUS_PROTOCOL_MSFT_PCI_VID,
		CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	CuAssertIntEquals (test, 0, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_init_null (CuTest *test)
{
	int status;
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;

	TEST_START;

	status = cmd_interface_mock_init (&cmd_interface);
	CuAssertIntEquals (test, 0, status);

	status = device_manager_init (&device_mgr, 1, DEVICE_MANAGER_AC_ROT_MODE,
		DEVICE_MANAGER_SLAVE_BUS_ROLE);
	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_init (NULL, &cmd_interface.base, &device_mgr,
		MCTP_PROTOCOL_PA_ROT_CTRL_EID, CERBERUS_PROTOCOL_MSFT_PCI_VID,
		CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	status = mctp_interface_init (&interface, NULL, &device_mgr,
		MCTP_PROTOCOL_PA_ROT_CTRL_EID, CERBERUS_PROTOCOL_MSFT_PCI_VID,
		CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	status = mctp_interface_init (&interface, &cmd_interface.base, NULL,
		MCTP_PROTOCOL_PA_ROT_CTRL_EID, CERBERUS_PROTOCOL_MSFT_PCI_VID,
		CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	status = cmd_interface_mock_validate_and_release (&cmd_interface);
	CuAssertIntEquals (test, 0, status);

	device_manager_release (&device_mgr);
}

static void mctp_interface_test_deinit_null (CuTest *test)
{
	TEST_START;

	mctp_interface_deinit (NULL);
}

static void mctp_interface_test_set_channel_id (CuTest *test)
{
	int status;
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_set_channel_id (&interface, 1);
	CuAssertIntEquals (test, 0, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_set_channel_id_null (CuTest *test)
{
	int status;

	TEST_START;

	status = mctp_interface_set_channel_id (NULL, 1);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);
}

static void mctp_interface_test_process_packet_null (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (NULL, &rx, &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	status = mctp_interface_process_packet (&interface, NULL, &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	status = mctp_interface_process_packet (&interface, &rx, NULL);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_invalid_req (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = 0x01;
	error->error_data = 0x7F001606;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_INVALID_REQ),
		MOCK_ARG (0x7F001606), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_INVALID_REQ, error->error_code);
	CuAssertIntEquals (test, 0x7F001606, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_unsupported_message (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = 0xAA;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = 0x01;
	error->error_data = 0x7F00160B;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_INVALID_REQ),
		MOCK_ARG (0x7F00160B), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_INVALID_REQ, error->error_code);
	CuAssertIntEquals (test, 0x7F00160B, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_invalid_crc (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = 0x00;
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_INVALID_CHECKSUM;
	error->error_data = checksum_crc8 (0xBA, rx.data, 17);

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_INVALID_CHECKSUM),
		MOCK_ARG (checksum_crc8 (0xBA, rx.data, 17)), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_INVALID_CHECKSUM, error->error_code);
	CuAssertIntEquals (test, checksum_crc8 (0xBA, rx.data, 17),	error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_packet_too_small (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	rx.pkt_size = 1;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TOO_SHORT, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_not_intended_target (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = 0x0C;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_interpret_fail_not_intended_target (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = 0x0C;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_out_of_order (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx[3];
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx[0].data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx[0].data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx[0].data[8] = 0x00;
	rx[0].data[9] = 0x00;
	rx[0].data[10] = 0x00;
	rx[0].data[17] = checksum_crc8 (0xBA, rx[0].data, 17);
	rx[0].pkt_size = 18;
	rx[0].dest_addr = 0x5D;

	header = (struct mctp_protocol_transport_header*) rx[1].data;

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 2;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	rx[1].pkt_size = 5;
	rx[1].dest_addr = 0x5D;

	header = (struct mctp_protocol_transport_header*) rx[2].data;

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 0;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 1;

	rx[2].data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx[2].data[8] = 0x00;
	rx[2].data[9] = 0x00;
	rx[2].data[10] = 0x00;
	rx[2].data[17] = checksum_crc8 (0xBA, rx[2].data, 17);
	rx[2].pkt_size = 18;
	rx[2].dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx[0], &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	status = mctp_interface_process_packet (&interface, &rx[1], &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TOO_SHORT, status);
	CuAssertPtrEquals (test, NULL, tx);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG),
		MOCK_ARG (0), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx[2], &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG, error->error_code);
	CuAssertIntEquals (test, 0, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_no_som (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (rx.data, 0, sizeof (rx.data));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 0;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG),
		MOCK_ARG (0), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG, error->error_code);
	CuAssertIntEquals (test, 0, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_invalid_msg_tag (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = 0x01;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_INVALID_REQ),
		MOCK_ARG (0), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->som = 0;
	header->eom = 0;
	header->tag_owner = 0;
	header->msg_tag = 0x01;
	header->packet_seq = 1;

	rx.data[6] = 0x11;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 1, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_INVALID_REQ, error->error_code);
	CuAssertIntEquals (test, 0, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_invalid_src_eid (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->source_eid = 0x0C;
	header->som = 0;
	header->eom = 1;
	header->packet_seq = 1;

	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_invalid_packet_seq (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_OUT_OF_SEQ_WINDOW;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->som = 0;
	header->packet_seq = 2;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_OUT_OF_SEQ_WINDOW),
		MOCK_ARG (0), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_OUT_OF_SEQ_WINDOW, error->error_code);
	CuAssertIntEquals (test, 0, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_invalid_msg_size (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_INVALID_PACKET_LEN;
	error->error_data = 9;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->byte_count = 14;
	header->som = 0;
	header->packet_seq = 1;

	rx.data[16] = checksum_crc8 (0xBA, rx.data, 16);
	rx.pkt_size = 17;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_INVALID_PACKET_LEN),
		MOCK_ARG (9), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_INVALID_PACKET_LEN, error->error_code);
	CuAssertIntEquals (test, 9, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_msg_overflow (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 237;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);
	rx.pkt_size = 240;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_MSG_OVERFLOW;
	error->error_data = 4097;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->som = 0;
	header->packet_seq = 1;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 2;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 3;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 0;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 1;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 2;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 3;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 0;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 1;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 2;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 3;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 0;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 1;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 2;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 3;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->packet_seq = 0;
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	header->byte_count = 158;
	header->packet_seq = 1;
	header->eom = 1;
	rx.data[160] = checksum_crc8 (0xBA, rx.data, 160);
	rx.pkt_size = 161;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_MSG_OVERFLOW),
		MOCK_ARG (4097), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_MSG_OVERFLOW, error->error_code);
	CuAssertIntEquals (test, 4097, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_cmd_interface_fail (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_UNSPECIFIED;
	error->error_data = CMD_HANDLER_PROCESS_FAILED;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		CMD_HANDLER_PROCESS_FAILED,
		MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_UNSPECIFIED),
		MOCK_ARG (CMD_HANDLER_PROCESS_FAILED), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, error->error_code);
	CuAssertIntEquals (test, CMD_HANDLER_PROCESS_FAILED, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_cmd_interface_fail_cmd_set_1 (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error =
		(struct cerberus_protocol_error*) &rx.data[MCTP_HEADER_LENGTH];
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	error->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	error->header.rq = 1;

	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error = (struct cerberus_protocol_error*) error_packet.data;

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 1;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_UNSPECIFIED;
	error->error_data = CMD_HANDLER_PROCESS_FAILED;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		CMD_HANDLER_PROCESS_FAILED,
		MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_UNSPECIFIED),
		MOCK_ARG (CMD_HANDLER_PROCESS_FAILED), MOCK_ARG (1));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 1, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, error->error_code);
	CuAssertIntEquals (test, CMD_HANDLER_PROCESS_FAILED, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_error_packet (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = CERBERUS_PROTOCOL_ERROR;
	rx.data[12] = CERBERUS_PROTOCOL_ERROR_INVALID_REQ;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		CMD_HANDLER_ERROR_MESSAGE,
		MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_no_response (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_NO_ERROR, error->error_code);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_no_response_non_zero_message_tag (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x02;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 2, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_NO_ERROR, error->error_code);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_no_response_cmd_set_1 (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error =
		(struct cerberus_protocol_error*) &rx.data[MCTP_HEADER_LENGTH];
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	error->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	error->header.rq = 1;

	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error = (struct cerberus_protocol_error*) error_packet.data;

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 1;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (1));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 1, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_NO_ERROR, error->error_code);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_unsupported_type (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = 0x0A;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = MCTP_PROTOCOL_UNSUPPORTED_MSG;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_INVALID_REQ;
	error->error_data = MCTP_PROTOCOL_UNSUPPORTED_MSG;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_INVALID_REQ),
		MOCK_ARG (MCTP_PROTOCOL_UNSUPPORTED_MSG), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_INVALID_REQ, error->error_code);
	CuAssertIntEquals (test, MCTP_PROTOCOL_UNSUPPORTED_MSG, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_mctp_control_msg (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_packet rx;
	struct cmd_message *tx;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct mctp_control_set_eid *rq = (struct mctp_control_set_eid*) &rx.data[MCTP_HEADER_LENGTH];
	struct mctp_control_set_eid_response *response;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = MCTP_HEADER_LENGTH + sizeof (struct mctp_control_set_eid) - 2;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rq->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_CONTROL_MSG;
	rq->header.command_code = MCTP_PROTOCOL_SET_EID;
	rq->header.rq = 1;

	rq->operation = MCTP_CONTROL_SET_EID_OPERATION_SET_ID;
	rq->eid = 0xAA;

	rx.pkt_size = MCTP_HEADER_LENGTH + sizeof (struct mctp_control_set_eid);
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_HEADER_LENGTH + sizeof (struct mctp_control_set_eid_response),
		tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	response = (struct mctp_control_set_eid_response*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 2, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);

	CuAssertIntEquals (test, 0, response->header.msg_type);
	CuAssertIntEquals (test, 1, response->header.command_code);
	CuAssertIntEquals (test, 0, response->header.rq);
	CuAssertIntEquals (test, 0, response->completion_code);
	CuAssertIntEquals (test, 1, response->eid_assignment_status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_mctp_control_msg_fail (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct mctp_protocol_control_header *ctrl_header =
		(struct mctp_protocol_control_header*) &rx.data[MCTP_HEADER_LENGTH];
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = sizeof (struct mctp_protocol_transport_header) +
		sizeof (struct mctp_protocol_control_header) - 2;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	ctrl_header->msg_type = MCTP_PROTOCOL_MSG_TYPE_CONTROL_MSG;
	ctrl_header->rsvd = 1;

	rx.pkt_size = sizeof (struct mctp_protocol_transport_header) +
		sizeof (struct mctp_protocol_control_header);
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_MSG, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_one_packet_request (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[2];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = sizeof (response_data);
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = true;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, 10, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 1, header->tag_owner);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, 0x7E, tx->data[7]);
	CuAssertIntEquals (test, 0x12, tx->data[8]);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_one_packet_response (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[2];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = sizeof (response_data);
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, 10, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, 7, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, 0x7E, tx->data[7]);
	CuAssertIntEquals (test, 0x12, tx->data[8]);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_one_packet_response_non_zero_message_tag (
	CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[2];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x03;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = sizeof (response_data);
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, 10, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, 7, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 3, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, 0x7E, tx->data[7]);
	CuAssertIntEquals (test, 0x12, tx->data[8]);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_two_packet_response (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT + 48];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;
	int first_pkt = MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT;
	int second_pkt = 48;
	int second_pkt_total = second_pkt + MCTP_PROTOCOL_PACKET_OVERHEAD;
	int response_size = first_pkt + second_pkt;
	int i;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response_data, 0, sizeof (response_data));
	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	for (i = 1; i < response_size; i++) {
		response.data[i] = i;
	}
	response.length = response_size;
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MAX_PACKET_LEN + second_pkt_total, tx->msg_size);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MAX_PACKET_LEN, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 0, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	status = testing_validate_array (response.data, &tx->data[MCTP_HEADER_LENGTH], first_pkt);
	CuAssertIntEquals (test, 0, status);

	header = (struct mctp_protocol_transport_header*) &tx->data[MCTP_PROTOCOL_MAX_PACKET_LEN];

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, second_pkt_total - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 0, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 1, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, &tx->data[tx->pkt_size], second_pkt_total - 1),
		tx->data[tx->msg_size - 1]);

	status = testing_validate_array (&response.data[first_pkt],
		&tx->data[MCTP_PROTOCOL_MAX_PACKET_LEN + MCTP_HEADER_LENGTH], second_pkt);
	CuAssertIntEquals (test, 0, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_channel_id_reset_next_som (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);
	error_packet.source_eid = 0x0B;
	error_packet.target_eid = 0x0A;
	error_packet.new_request = false;
	error_packet.crypto_timeout = false;
	error_packet.channel_id = 1;
	error_packet.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_set_channel_id (&interface, 1);
	CuAssertIntEquals (test, 0, status);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_NO_ERROR, error->error_code);
	CuAssertIntEquals (test, 0, error->error_data);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_normal_timeout (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[2];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;
	rx.timeout_valid = true;
	platform_init_timeout (10, &rx.pkt_timeout);

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = sizeof (response_data);
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	platform_msleep (20);
	CuAssertIntEquals (test, true, platform_has_timeout_expired (&rx.pkt_timeout));

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, 10, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, 0x7E, tx->data[7]);
	CuAssertIntEquals (test, 0x12, tx->data[8]);
	CuAssertIntEquals (test, true, platform_has_timeout_expired (&rx.pkt_timeout));

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_crypto_timeout (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[2];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;
	rx.timeout_valid = true;
	platform_init_timeout (10, &rx.pkt_timeout);

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = sizeof (response_data);
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = true;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	platform_msleep (20);
	CuAssertIntEquals (test, true, platform_has_timeout_expired (&rx.pkt_timeout));

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, 10, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, 7, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, 0x7E, tx->data[7]);
	CuAssertIntEquals (test, 0x12, tx->data[8]);
	CuAssertIntEquals (test, false, platform_has_timeout_expired (&rx.pkt_timeout));

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_max_message (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	uint8_t msg_data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	int status;
	int i;

	TEST_START;

	msg_data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;

	for (i = 1; i < (int) sizeof (msg_data); i++) {
		msg_data[i] = i;
	}

	i = 0;
	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 237;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);
	rx.pkt_size = 240;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->som = 0;
	header->packet_seq = 1;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 2;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 3;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 0;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 1;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 2;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 3;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 0;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 1;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 2;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 3;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 0;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 1;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 2;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 3;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->packet_seq = 0;
	memcpy (&rx.data[7], &msg_data[i], 232);
	rx.data[239] = checksum_crc8 (0xBA, rx.data, 239);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	i += 232;
	header->byte_count = 157;
	header->packet_seq = 1;
	header->eom = 1;
	memcpy (&rx.data[7], &msg_data[i], 152);
	rx.data[159] = checksum_crc8 (0xBA, rx.data, 159);
	rx.pkt_size = 160;

	request.data = data;
	request.length = sizeof (msg_data);
	memcpy (request.data, msg_data, request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_NO_ERROR, error->error_code);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_max_response (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	size_t max_packets =
		ceil ((MCTP_PROTOCOL_MAX_MESSAGE_BODY * 1.0) / MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT);
	size_t remain =
		MCTP_PROTOCOL_MAX_MESSAGE_BODY - (MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT * (max_packets - 1));
	int status;
	size_t i;
	size_t pkt_size = MCTP_PROTOCOL_MAX_PACKET_LEN;
	size_t last_pkt_size = remain + MCTP_PROTOCOL_PACKET_OVERHEAD;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response_data, 0, sizeof (response_data));
	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	for (i = 1; i < MCTP_PROTOCOL_MAX_MESSAGE_BODY; i++) {
		response.data[i] = i;
	}
	response.length = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	CuAssertIntEquals (test, max_packets, MCTP_PROTOCOL_MAX_PACKET_PER_MESSAGE);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test,
		MCTP_PROTOCOL_MAX_MESSAGE_BODY + (MCTP_PROTOCOL_PACKET_OVERHEAD * max_packets),
		tx->msg_size);
	CuAssertIntEquals (test, pkt_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	for (i = 0; i < max_packets - 1; i++) {
		header =
			(struct mctp_protocol_transport_header*) &tx->data[i * MCTP_PROTOCOL_MAX_PACKET_LEN];

		CuAssertIntEquals (test, 0x0F, header->cmd_code);
		CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
		CuAssertIntEquals (test, 0xBB, header->source_addr);
		CuAssertIntEquals (test, 0x0A, header->destination_eid);
		CuAssertIntEquals (test, 0x0B, header->source_eid);
		CuAssertIntEquals (test, !i, header->som);
		CuAssertIntEquals (test, 0, header->eom);
		CuAssertIntEquals (test, 0, header->tag_owner);
		CuAssertIntEquals (test, 0, header->msg_tag);
		CuAssertIntEquals (test, i % 4, header->packet_seq);
		CuAssertIntEquals (test,
			checksum_crc8 (0xAA, &tx->data[i * tx->pkt_size], tx->pkt_size - 1),
			tx->data[((i + 1) * tx->pkt_size) - 1]);

		status = testing_validate_array (&response.data[i * MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT],
			&tx->data[(i * pkt_size) + MCTP_HEADER_LENGTH], MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT);
		CuAssertIntEquals (test, 0, status);
	}

	header = (struct mctp_protocol_transport_header*) &tx->data[i * pkt_size];

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, remain + MCTP_PROTOCOL_PACKET_OVERHEAD - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 0, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, i % 4, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, &tx->data[i * tx->pkt_size], last_pkt_size - 1),
		tx->data[tx->msg_size - 1]);

	status = testing_validate_array (&response.data[i * MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT],
		&tx->data[(i * pkt_size) + MCTP_HEADER_LENGTH], remain);
	CuAssertIntEquals (test, 0, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_max_response_min_packets (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct device_manager_full_capabilities remote;
	size_t max_packets =
		ceil ((MCTP_PROTOCOL_MAX_MESSAGE_BODY * 1.0) / MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT);
	size_t remain =
		MCTP_PROTOCOL_MAX_MESSAGE_BODY - (MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT * (max_packets - 1));
	int status;
	size_t i;
	size_t pkt_size = MCTP_PROTOCOL_MIN_PACKET_LEN;
	size_t last_pkt_size = remain + MCTP_PROTOCOL_PACKET_OVERHEAD;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	memset (&remote, 0, sizeof (remote));
	remote.request.max_message_size = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	remote.request.max_packet_size = MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT;
	remote.request.security_mode = DEVICE_MANAGER_SECURITY_AUTHENTICATION;
	remote.request.bus_role = DEVICE_MANAGER_SLAVE_BUS_ROLE;
	remote.request.hierarchy_role = DEVICE_MANAGER_AC_ROT_MODE;
	remote.max_timeout = MCTP_PROTOCOL_MAX_RESPONSE_TIMEOUT_MS / 10;
	remote.max_sig = MCTP_PROTOCOL_MAX_CRYPTO_TIMEOUT_MS / 100;

	status = device_manager_update_device_capabilities (&device_mgr, 1, &remote);
	CuAssertIntEquals (test, 0, status);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response_data, 0, sizeof (response_data));
	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	for (i = 1; i < MCTP_PROTOCOL_MAX_MESSAGE_BODY; i++) {
		response.data[i] = i;
	}
	response.length = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	CuAssertIntEquals (test, max_packets,
		MCTP_PROTOCOL_PACKETS_IN_MESSAGE (MCTP_PROTOCOL_MAX_MESSAGE_BODY,
			MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT));
	CuAssertIntEquals (test, sizeof (interface.msg_buffer),
		MCTP_PROTOCOL_MAX_MESSAGE_BODY + (MCTP_PROTOCOL_PACKET_OVERHEAD * max_packets));

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test,
		MCTP_PROTOCOL_MAX_MESSAGE_BODY + (MCTP_PROTOCOL_PACKET_OVERHEAD * max_packets),
		tx->msg_size);
	CuAssertIntEquals (test, pkt_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	for (i = 0; i < max_packets - 1; i++) {
		header =
			(struct mctp_protocol_transport_header*) &tx->data[i * pkt_size];

		CuAssertIntEquals (test, 0x0F, header->cmd_code);
		CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
		CuAssertIntEquals (test, 0xBB, header->source_addr);
		CuAssertIntEquals (test, 0x0A, header->destination_eid);
		CuAssertIntEquals (test, 0x0B, header->source_eid);
		CuAssertIntEquals (test, !i, header->som);
		CuAssertIntEquals (test, 0, header->eom);
		CuAssertIntEquals (test, 0, header->tag_owner);
		CuAssertIntEquals (test, 0, header->msg_tag);
		CuAssertIntEquals (test, i % 4, header->packet_seq);
		CuAssertIntEquals (test,
			checksum_crc8 (0xAA, &tx->data[i * tx->pkt_size], tx->pkt_size - 1),
			tx->data[((i + 1) * tx->pkt_size) - 1]);

		status = testing_validate_array (&response.data[i * MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT],
			&tx->data[(i * pkt_size) + MCTP_HEADER_LENGTH], MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT);
		CuAssertIntEquals (test, 0, status);
	}

	header = (struct mctp_protocol_transport_header*) &tx->data[i * pkt_size];

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, remain + MCTP_PROTOCOL_PACKET_OVERHEAD - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 0, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, i % 4, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, &tx->data[i * tx->pkt_size], last_pkt_size - 1),
		tx->data[tx->msg_size - 1]);

	status = testing_validate_array (&response.data[i * MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT],
		&tx->data[(i * pkt_size) + MCTP_HEADER_LENGTH], remain);
	CuAssertIntEquals (test, 0, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_no_eom (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_reset_message_processing (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx[2];
	struct cmd_message *tx;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx[0].data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 0;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx[0].data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx[0].data[8] = 0x00;
	rx[0].data[9] = 0x00;
	rx[0].data[10] = 0x00;
	rx[0].data[17] = checksum_crc8 (0xBA, rx[0].data, 17);
	rx[0].pkt_size = 18;
	rx[0].dest_addr = 0x5D;

	header = (struct mctp_protocol_transport_header*) rx[1].data;

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 0;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx[1].data[7] = 0x00;
	rx[1].data[8] = 0x00;
	rx[1].data[9] = 0x00;
	rx[1].data[10] = 0x00;
	rx[1].data[17] = checksum_crc8 (0xBA, rx[1].data, 17);
	rx[1].pkt_size = 18;
	rx[1].dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_process_packet (&interface, &rx[0], &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrEquals (test, NULL, tx);

	mctp_interface_reset_message_processing (&interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG),
		MOCK_ARG (0), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx[1], &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG, error->error_code);
	CuAssertIntEquals (test, 0, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_response_length_limited (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	struct device_manager_full_capabilities remote;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	memset (&remote, 0, sizeof (remote));
	remote.request.max_message_size = MCTP_PROTOCOL_MAX_MESSAGE_BODY - 128;
	remote.request.max_packet_size = MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT;
	remote.request.security_mode = DEVICE_MANAGER_SECURITY_AUTHENTICATION;
	remote.request.bus_role = DEVICE_MANAGER_SLAVE_BUS_ROLE;
	remote.request.hierarchy_role = DEVICE_MANAGER_AC_ROT_MODE;
	remote.max_timeout = MCTP_PROTOCOL_MAX_RESPONSE_TIMEOUT_MS / 10;
	remote.max_sig = MCTP_PROTOCOL_MAX_CRYPTO_TIMEOUT_MS / 100;

	status = device_manager_update_device_capabilities (&device_mgr, 1, &remote);
	CuAssertIntEquals (test, 0, status);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY - 128;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_NO_ERROR, error->error_code);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_response_too_large (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[MCTP_PROTOCOL_MAX_MESSAGE_BODY + 1];
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_UNSPECIFIED;
	error->error_data = 0x7F001605;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = MCTP_PROTOCOL_MAX_MESSAGE_BODY + 1;
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_UNSPECIFIED),
		MOCK_ARG (0x7F001605), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, error->error_code);
	CuAssertIntEquals (test, 0x7F001605, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_response_too_large_length_limited (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	struct device_manager_full_capabilities remote;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_ERROR_UNSPECIFIED;
	error->error_data = 0x7F001605;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	memset (&remote, 0, sizeof (remote));
	remote.request.max_message_size = MCTP_PROTOCOL_MAX_MESSAGE_BODY - 128;
	remote.request.max_packet_size = MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT;
	remote.request.security_mode = DEVICE_MANAGER_SECURITY_AUTHENTICATION;
	remote.request.bus_role = DEVICE_MANAGER_SLAVE_BUS_ROLE;
	remote.request.hierarchy_role = DEVICE_MANAGER_AC_ROT_MODE;
	remote.max_timeout = MCTP_PROTOCOL_MAX_RESPONSE_TIMEOUT_MS / 10;
	remote.max_sig = MCTP_PROTOCOL_MAX_CRYPTO_TIMEOUT_MS / 100;

	status = device_manager_update_device_capabilities (&device_mgr, 1, &remote);
	CuAssertIntEquals (test, 0, status);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY - 128;

	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	response.data[1] = 0x12;
	response.length = remote.request.max_message_size + 1;
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_ERROR_UNSPECIFIED),
		MOCK_ARG (0x7F001605), MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, MCTP_ERROR_MSG_LENGTH, tx->msg_size);
	CuAssertIntEquals (test, tx->msg_size, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;
	error = (struct cerberus_protocol_error*) &tx->data[MCTP_HEADER_LENGTH];

	CuAssertIntEquals (test, SMBUS_CMD_CODE_MCTP, header->cmd_code);
	CuAssertIntEquals (test, tx->pkt_size - 3, header->byte_count);
	CuAssertIntEquals (test, 0x5D << 1 | 1, header->source_addr);
	CuAssertIntEquals (test, 0, header->rsvd);
	CuAssertIntEquals (test, 1, header->header_version);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BMC_EID, header->destination_eid);
	CuAssertIntEquals (test, MCTP_PROTOCOL_PA_ROT_CTRL_EID, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, error->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, error->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, error->header.crypt);
	CuAssertIntEquals (test, 0, error->header.reserved2);
	CuAssertIntEquals (test, 0, error->header.integrity_check);
	CuAssertIntEquals (test, 0, error->header.reserved1);
	CuAssertIntEquals (test, 0, error->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR, error->header.command);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, error->error_code);
	CuAssertIntEquals (test, 0x7F001605, error->error_data);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_two_packet_response_length_limited (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[48 + 10];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct device_manager_full_capabilities remote;
	int status;
	int first_pkt = 48;	// This is not a valid max packet size, but ensures test portability.
	int first_pkt_total = first_pkt + MCTP_PROTOCOL_PACKET_OVERHEAD;
	int second_pkt = 10;
	int second_pkt_total = second_pkt + MCTP_PROTOCOL_PACKET_OVERHEAD;
	int response_size = first_pkt + second_pkt;
	int i;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	memset (&remote, 0, sizeof (remote));
	remote.request.max_message_size = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	remote.request.max_packet_size = first_pkt;
	remote.request.security_mode = DEVICE_MANAGER_SECURITY_AUTHENTICATION;
	remote.request.bus_role = DEVICE_MANAGER_SLAVE_BUS_ROLE;
	remote.request.hierarchy_role = DEVICE_MANAGER_AC_ROT_MODE;
	remote.max_timeout = MCTP_PROTOCOL_MAX_RESPONSE_TIMEOUT_MS / 10;
	remote.max_sig = MCTP_PROTOCOL_MAX_CRYPTO_TIMEOUT_MS / 100;

	status = device_manager_update_device_capabilities (&device_mgr, 1, &remote);
	CuAssertIntEquals (test, 0, status);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (response_data, 0, sizeof (response_data));
	response.data = response_data;
	response.data[0] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	for (i = 1; i < response_size; i++) {
		response.data[i] = i;
	}
	response.length = response_size;
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, 0, status);
	CuAssertPtrNotNull (test, tx);

	CuAssertIntEquals (test, first_pkt_total + second_pkt_total, tx->msg_size);
	CuAssertIntEquals (test, first_pkt_total, tx->pkt_size);
	CuAssertIntEquals (test, 0x55, tx->dest_addr);

	header = (struct mctp_protocol_transport_header*) tx->data;

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, first_pkt_total - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 0, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, tx->data, tx->pkt_size - 1),
		tx->data[tx->pkt_size - 1]);

	status = testing_validate_array (response.data, &tx->data[MCTP_HEADER_LENGTH], first_pkt);
	CuAssertIntEquals (test, 0, status);

	header = (struct mctp_protocol_transport_header*) &tx->data[first_pkt_total];

	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, second_pkt_total - 3, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 0, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->tag_owner);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 1, header->packet_seq);
	CuAssertIntEquals (test, checksum_crc8 (0xAA, &tx->data[first_pkt_total], second_pkt_total - 1),
		tx->data[tx->msg_size - 1]);

	status = testing_validate_array (&response.data[first_pkt],
		&tx->data[first_pkt_total + MCTP_HEADER_LENGTH], second_pkt);
	CuAssertIntEquals (test, 0, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_error_message_fail (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, CMD_HANDLER_ERROR_MSG_FAILED, MOCK_ARG_NOT_NULL,
		MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0), MOCK_ARG (0));

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, CMD_HANDLER_ERROR_MSG_FAILED, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_error_too_large (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT + 1];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x7E;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TOO_LARGE, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_error_message_type_unsupported (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	struct cmd_interface_request response;
	uint8_t error_data[sizeof (struct cerberus_protocol_error)];
	struct cmd_interface_request error_packet;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	struct cerberus_protocol_error *error = (struct cerberus_protocol_error*) error_data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 1;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	error_packet.data = error_data;
	error_packet.length = sizeof (error_data);

	error->header.msg_type = 0x55;
	error->header.pci_vendor_id = 0x1414;
	error->header.crypt = 0;
	error->header.reserved2 = 0;
	error->header.integrity_check = 0;
	error->header.reserved1 = 0;
	error->header.rq = 0;
	error->header.command = 0x7F;
	error->error_code = CERBERUS_PROTOCOL_NO_ERROR;
	error->error_data = 0;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	memset (&response, 0, sizeof (response));
	response.data = data;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	status |= mock_expect (&cmd_interface.mock, cmd_interface.base.generate_error_packet,
		&cmd_interface, 0, MOCK_ARG_NOT_NULL, MOCK_ARG (CERBERUS_PROTOCOL_NO_ERROR), MOCK_ARG (0),
		MOCK_ARG (0));
	status |= mock_expect_output (&cmd_interface.mock, 0, &error_packet, sizeof (error_packet), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BUILD_UNSUPPORTED, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_process_packet_response_message_type_unsupported (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cmd_packet rx;
	struct cmd_message *tx;
	uint8_t data[10];
	struct cmd_interface_request request;
	uint8_t response_data[2];
	struct cmd_interface_request response;
	struct mctp_protocol_transport_header *header =
		(struct mctp_protocol_transport_header*) rx.data;
	int status;

	TEST_START;

	memset (&rx, 0, sizeof (rx));

	header->cmd_code = SMBUS_CMD_CODE_MCTP;
	header->byte_count = 15;
	header->source_addr = 0xAB;
	header->rsvd = 0;
	header->header_version = 1;
	header->destination_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	header->source_eid = MCTP_PROTOCOL_BMC_EID;
	header->som = 1;
	header->eom = 1;
	header->tag_owner = 0;
	header->msg_tag = 0x00;
	header->packet_seq = 0;

	rx.data[7] = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	rx.data[8] = 0x00;
	rx.data[9] = 0x00;
	rx.data[10] = 0x00;
	rx.data[11] = 0x01;
	rx.data[12] = 0x02;
	rx.data[13] = 0x03;
	rx.data[14] = 0x04;
	rx.data[15] = 0x05;
	rx.data[16] = 0x06;
	rx.data[17] = checksum_crc8 (0xBA, rx.data, 17);
	rx.pkt_size = 18;
	rx.dest_addr = 0x5D;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	request.data = data;
	request.length = sizeof (data);
	memcpy (request.data, &rx.data[7], request.length);
	request.source_eid = 0x0A;
	request.target_eid = 0x0B;
	request.new_request = false;
	request.crypto_timeout = false;
	request.channel_id = 0;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;

	response.data = response_data;
	response.data[0] = 0x11;
	response.data[1] = 0x22;
	response.length = sizeof (response_data);
	response.source_eid = 0x0A;
	response.target_eid = 0x0B;
	response.new_request = false;
	response.crypto_timeout = false;

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.process_request, &cmd_interface,
		0, MOCK_ARG_VALIDATOR_DEEP_COPY (cmd_interface_mock_validate_request, &request,
			sizeof (request), cmd_interface_mock_save_request, cmd_interface_mock_free_request));
	status |= mock_expect_output (&cmd_interface.mock, 0, &response, sizeof (response), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_process_packet (&interface, &rx, &tx);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BUILD_UNSUPPORTED, status);
	CuAssertPtrEquals (test, NULL, tx);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	uint8_t params;
	uint8_t request[5] = {MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, 1, 2, 3, 4};
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct mctp_protocol_transport_header *header = (struct mctp_protocol_transport_header*) buf;
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.issue_request, &cmd_interface,
		sizeof (request), MOCK_ARG (CERBERUS_PROTOCOL_GET_CERTIFICATE), MOCK_ARG (&params),
		MOCK_ARG_NOT_NULL, MOCK_ARG (MCTP_PROTOCOL_MAX_MESSAGE_BODY));
	status |= mock_expect_output (&cmd_interface.mock, 2, request, sizeof (request), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B,
		CERBERUS_PROTOCOL_GET_CERTIFICATE, &params, buf, sizeof (buf),
		MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF);
	CuAssertIntEquals (test, 13, status);
	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, 10, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0xFF, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, 1, header->tag_owner);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, buf[7]);
	CuAssertIntEquals (test, 1, buf[8]);
	CuAssertIntEquals (test, 2, buf[9]);
	CuAssertIntEquals (test, 3, buf[10]);
	CuAssertIntEquals (test, 4, buf[11]);
	CuAssertIntEquals (test, checksum_crc8 (0xEE, buf, 12), buf[12]);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_limited_message_length (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct device_manager_full_capabilities remote;
	uint8_t params;
	uint8_t request[5] = {MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, 1, 2, 3, 4};
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct mctp_protocol_transport_header *header = (struct mctp_protocol_transport_header*) buf;
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	memset (&remote, 0, sizeof (remote));
	remote.request.max_message_size = MCTP_PROTOCOL_MAX_MESSAGE_BODY - 128;
	remote.request.max_packet_size = MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT;
	remote.request.security_mode = DEVICE_MANAGER_SECURITY_AUTHENTICATION;
	remote.request.bus_role = DEVICE_MANAGER_SLAVE_BUS_ROLE;
	remote.request.hierarchy_role = DEVICE_MANAGER_AC_ROT_MODE;
	remote.max_timeout = MCTP_PROTOCOL_MAX_RESPONSE_TIMEOUT_MS / 10;
	remote.max_sig = MCTP_PROTOCOL_MAX_CRYPTO_TIMEOUT_MS / 100;

	status = device_manager_update_device_capabilities (&device_mgr, 1, &remote);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.issue_request, &cmd_interface,
		sizeof (request), MOCK_ARG (CERBERUS_PROTOCOL_GET_CERTIFICATE), MOCK_ARG (&params),
		MOCK_ARG_NOT_NULL, MOCK_ARG (MCTP_PROTOCOL_MAX_MESSAGE_BODY - 128));
	status |= mock_expect_output (&cmd_interface.mock, 2, request, sizeof (request), -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_issue_request (&interface, 0x77, 0x0A, 0x5D, 0x0B,
		CERBERUS_PROTOCOL_GET_CERTIFICATE, &params, buf, sizeof (buf),
		MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF);
	CuAssertIntEquals (test, 13, status);
	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, 10, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0x0A, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, 1, header->tag_owner);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, buf[7]);
	CuAssertIntEquals (test, 1, buf[8]);
	CuAssertIntEquals (test, 2, buf[9]);
	CuAssertIntEquals (test, 3, buf[10]);
	CuAssertIntEquals (test, 4, buf[11]);
	CuAssertIntEquals (test, checksum_crc8 (0xEE, buf, 12), buf[12]);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_mctp_ctrl_msg (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	uint8_t params = 0xAA;
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct mctp_protocol_transport_header *header = (struct mctp_protocol_transport_header*) buf;
	struct mctp_control_set_eid *request = (struct mctp_control_set_eid*) &buf[MCTP_HEADER_LENGTH];
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B,
		MCTP_PROTOCOL_SET_EID, &params, buf, sizeof (buf), MCTP_PROTOCOL_MSG_TYPE_CONTROL_MSG);
	CuAssertIntEquals (test, 12, status);
	CuAssertIntEquals (test, 0x0F, header->cmd_code);
	CuAssertIntEquals (test, 10, header->byte_count);
	CuAssertIntEquals (test, 0xBB, header->source_addr);
	CuAssertIntEquals (test, 0xFF, header->destination_eid);
	CuAssertIntEquals (test, 0x0B, header->source_eid);
	CuAssertIntEquals (test, 1, header->som);
	CuAssertIntEquals (test, 1, header->eom);
	CuAssertIntEquals (test, 0, header->msg_tag);
	CuAssertIntEquals (test, 0, header->packet_seq);
	CuAssertIntEquals (test, 1, header->tag_owner);
	CuAssertIntEquals (test, 0, request->header.msg_type);
	CuAssertIntEquals (test, 1, request->header.command_code);
	CuAssertIntEquals (test, 1, request->header.rq);
	CuAssertIntEquals (test, 0, request->header.rsvd);
	CuAssertIntEquals (test, 0, request->header.instance_id);
	CuAssertIntEquals (test, 0, request->header.integrity_check);
	CuAssertIntEquals (test, 0, request->header.d_bit);
	CuAssertIntEquals (test, 0, request->operation);
	CuAssertIntEquals (test, params, request->eid);
	CuAssertIntEquals (test, 0, request->reserved);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_null (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	struct cerberus_protocol_cert_req_params params = {0};
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_issue_request (NULL, 0x77, 0xFF, 0x5D, 0x0B, 0x82, &params,
		buf, sizeof (buf), MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B, 0x82, &params,
		NULL, sizeof (buf), MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF);
	CuAssertIntEquals (test, MCTP_PROTOCOL_INVALID_ARGUMENT, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_fail (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.issue_request, &cmd_interface,
		CMD_HANDLER_NO_MEMORY, MOCK_ARG (CERBERUS_PROTOCOL_GET_CERTIFICATE), MOCK_ARG_ANY,
		MOCK_ARG_NOT_NULL, MOCK_ARG (MCTP_PROTOCOL_MAX_MESSAGE_BODY));
	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B,
		CERBERUS_PROTOCOL_GET_CERTIFICATE, NULL, buf, sizeof (buf),
		MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF);
	CuAssertIntEquals (test, CMD_HANDLER_NO_MEMORY, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_mctp_ctrl_msg_fail (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	uint8_t params;
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B, 0xFF,
		(void*) &params, buf, sizeof (buf), MCTP_PROTOCOL_MSG_TYPE_CONTROL_MSG);
	CuAssertIntEquals (test, CMD_HANDLER_UNKNOWN_COMMAND, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_unsupported_msg_type (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B,
		CERBERUS_PROTOCOL_GET_CERTIFICATE, NULL, buf, sizeof (buf), 0xFF);
	CuAssertIntEquals (test, MCTP_PROTOCOL_UNSUPPORTED_MSG, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}

static void mctp_interface_test_issue_request_construct_packet_fail (CuTest *test)
{
	struct mctp_interface interface;
	struct cmd_interface_mock cmd_interface;
	struct device_manager device_mgr;
	uint8_t cert_digest_request[5] = {MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, 1, 2, 3, 4};
 	uint8_t buf[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	int status;

	TEST_START;

	setup_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr, &interface);

	status = mock_expect (&cmd_interface.mock, cmd_interface.base.issue_request, &cmd_interface,
		MCTP_PROTOCOL_MAX_MESSAGE_BODY + 1, MOCK_ARG (CERBERUS_PROTOCOL_GET_CERTIFICATE),
		MOCK_ARG_ANY, MOCK_ARG_NOT_NULL, MOCK_ARG (MCTP_PROTOCOL_MAX_MESSAGE_BODY));
	status |= mock_expect_output (&cmd_interface.mock, 2, cert_digest_request, 5, -1);

	CuAssertIntEquals (test, 0, status);

	status = mctp_interface_issue_request (&interface, 0x77, 0xFF, 0x5D, 0x0B,
		CERBERUS_PROTOCOL_GET_CERTIFICATE, &cert_digest_request, buf, sizeof (buf),
		MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF);
	CuAssertIntEquals (test, MCTP_PROTOCOL_BAD_BUFFER_LENGTH, status);

	complete_mctp_interface_with_interface_mock_test (test, &cmd_interface, &device_mgr,
		&interface);
}


CuSuite* get_mctp_interface_suite ()
{
	CuSuite *suite = CuSuiteNew ();

	SUITE_ADD_TEST (suite, mctp_interface_test_init);
	SUITE_ADD_TEST (suite, mctp_interface_test_init_null);
	SUITE_ADD_TEST (suite, mctp_interface_test_deinit_null);
	SUITE_ADD_TEST (suite, mctp_interface_test_set_channel_id);
	SUITE_ADD_TEST (suite, mctp_interface_test_set_channel_id_null);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_null);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_invalid_req);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_unsupported_message);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_invalid_crc);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_packet_too_small);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_not_intended_target);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_interpret_fail_not_intended_target);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_out_of_order);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_no_som);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_invalid_msg_tag);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_invalid_src_eid);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_invalid_packet_seq);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_invalid_msg_size);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_msg_overflow);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_cmd_interface_fail);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_cmd_interface_fail_cmd_set_1);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_error_packet);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_no_response);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_no_response_non_zero_message_tag);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_no_response_cmd_set_1);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_unsupported_type);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_mctp_control_msg);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_mctp_control_msg_fail);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_one_packet_request);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_one_packet_response);
	SUITE_ADD_TEST (suite,
		mctp_interface_test_process_packet_one_packet_response_non_zero_message_tag);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_two_packet_response);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_channel_id_reset_next_som);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_normal_timeout);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_crypto_timeout);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_max_message);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_max_response);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_max_response_min_packets);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_no_eom);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_reset_message_processing);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_response_length_limited);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_response_too_large);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_response_too_large_length_limited);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_two_packet_response_length_limited);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_error_message_fail);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_error_too_large);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_error_message_type_unsupported);
	SUITE_ADD_TEST (suite, mctp_interface_test_process_packet_response_message_type_unsupported);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_limited_message_length);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_mctp_ctrl_msg);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_null);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_fail);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_mctp_ctrl_msg_fail);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_unsupported_msg_type);
	SUITE_ADD_TEST (suite, mctp_interface_test_issue_request_construct_packet_fail);

	return suite;
}
