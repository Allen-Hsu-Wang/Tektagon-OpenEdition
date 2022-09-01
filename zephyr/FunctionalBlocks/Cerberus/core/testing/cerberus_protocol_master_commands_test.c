// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "testing.h"
#include "cmd_interface/cerberus_protocol.h"
#include "cmd_interface/cerberus_protocol_master_commands.h"
#include "cmd_interface/cerberus_protocol_required_commands.h"
#include "flash/flash_updater.h"
#include "mock/cfm_mock.h"
#include "mock/pcd_mock.h"
#include "cerberus_protocol_master_commands_testing.h"
#include "pcd_testing.h"
#include "cfm_testing.h"


static const char *SUITE = "cerberus_protocol_master_commands";


void cerberus_protocol_master_commands_testing_process_process_certificate_digest (CuTest *test,
	struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_challenge *req = (struct cerberus_protocol_challenge*) data;
	struct cerberus_protocol_get_certificate_digest_response *resp =
		(struct cerberus_protocol_get_certificate_digest_response*) data;
	struct attestation_challenge challenge = {0};
	struct attestation_chain_digest expected;
	int status;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG;

	expected.digest = &data[sizeof (struct cerberus_protocol_get_certificate_digest_response)];
	expected.digest_len = SHA256_HASH_LENGTH;
	expected.num_cert = 3;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_DIGEST;

	resp->capabilities = 1;
	resp->num_digests = 3;
	request.length = sizeof (struct cerberus_protocol_get_certificate_digest_response) +
		(SHA256_HASH_LENGTH * 3);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	challenge.nonce[0] = 0xAA;
	challenge.nonce[31] = 0xBB;

	status = mock_expect (&master_attestation->mock, master_attestation->base.compare_digests,
		master_attestation, 0, MOCK_ARG (MCTP_PROTOCOL_BMC_EID),
		MOCK_ARG_PTR_CONTAINS_TMP (&expected, 3));

	status |= mock_expect (&master_attestation->mock, master_attestation->base.issue_challenge,
		master_attestation, sizeof (struct attestation_challenge), MOCK_ARG (MCTP_PROTOCOL_BMC_EID),
		MOCK_ARG (0), MOCK_ARG (&request.data[CERBERUS_PROTOCOL_MIN_MSG_LEN]), MOCK_ARG (max));
	status |= mock_expect_output (&master_attestation->mock, 2, &challenge,
		sizeof (struct attestation_challenge), -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = false;
	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_challenge), request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE, req->header.command);
	CuAssertIntEquals (test, true, request.new_request);
	CuAssertIntEquals (test, true, request.crypto_timeout);

	status = testing_validate_array ((uint8_t*) &challenge, (uint8_t*) &req->challenge,
		sizeof (challenge));
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_process_certificate_digest_cert_mismatch (
	CuTest *test, struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate *req =
		(struct cerberus_protocol_get_certificate*) data;
	struct cerberus_protocol_get_certificate_digest_response *resp =
		(struct cerberus_protocol_get_certificate_digest_response*) data;
	struct attestation_chain_digest expected;
	int status;

	expected.digest = &data[sizeof (struct cerberus_protocol_get_certificate_digest_response)];
	expected.digest_len = SHA256_HASH_LENGTH;
	expected.num_cert = 3;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_DIGEST;

	resp->capabilities = 1;
	resp->num_digests = 3;
	request.length = sizeof (struct cerberus_protocol_get_certificate_digest_response) +
		(SHA256_HASH_LENGTH * 3);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock, master_attestation->base.compare_digests,
		master_attestation, 1, MOCK_ARG (MCTP_PROTOCOL_BMC_EID),
		MOCK_ARG_PTR_CONTAINS_TMP (&expected, 3));

	CuAssertIntEquals (test, 0, status);

	request.new_request = false;
	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_certificate), request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CERTIFICATE, req->header.command);
	CuAssertIntEquals (test, 0, req->certificate.cert_num);
	CuAssertIntEquals (test, 0, req->certificate.slot_num);
	CuAssertIntEquals (test, 0, req->certificate.offset);
	CuAssertIntEquals (test, 0, req->certificate.length);
	CuAssertIntEquals (test, true, request.new_request);
	CuAssertIntEquals (test, true, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_certificate_digest_digests_fail (
	CuTest *test, struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate_digest_response *resp =
		(struct cerberus_protocol_get_certificate_digest_response*) data;
	struct attestation_chain_digest expected;
	int status;

	expected.digest = &data[sizeof (struct cerberus_protocol_get_certificate_digest_response)];
	expected.digest_len = SHA256_HASH_LENGTH;
	expected.num_cert = 3;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_DIGEST;

	resp->capabilities = 1;
	resp->num_digests = 3;
	request.length = sizeof (struct cerberus_protocol_get_certificate_digest_response) +
		(SHA256_HASH_LENGTH * 3);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock, master_attestation->base.compare_digests,
		master_attestation, ATTESTATION_NO_MEMORY, MOCK_ARG (MCTP_PROTOCOL_BMC_EID),
		MOCK_ARG_PTR_CONTAINS_TMP (&expected, 3));

	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, ATTESTATION_NO_MEMORY, status);
	CuAssertIntEquals (test, true, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_certificate_digest_challenge_fail (
	CuTest *test, struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate_digest_response *resp =
		(struct cerberus_protocol_get_certificate_digest_response*) data;
	struct attestation_chain_digest expected;
	int status;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG;

	expected.digest = &data[sizeof (struct cerberus_protocol_get_certificate_digest_response)];
	expected.digest_len = SHA256_HASH_LENGTH;
	expected.num_cert = 3;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_DIGEST;

	resp->capabilities = 1;
	resp->num_digests = 3;
	request.length = sizeof (struct cerberus_protocol_get_certificate_digest_response) +
		(SHA256_HASH_LENGTH * 3);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock, master_attestation->base.compare_digests,
		master_attestation, 0, MOCK_ARG (MCTP_PROTOCOL_BMC_EID),
		MOCK_ARG_PTR_CONTAINS_TMP (&expected, 3));

	status |= mock_expect (&master_attestation->mock, master_attestation->base.issue_challenge,
		master_attestation, ATTESTATION_NO_MEMORY, MOCK_ARG (MCTP_PROTOCOL_BMC_EID), MOCK_ARG (0),
		MOCK_ARG (&request.data[CERBERUS_PROTOCOL_MIN_MSG_LEN]), MOCK_ARG (max));

	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, ATTESTATION_NO_MEMORY, status);
	CuAssertIntEquals (test, true, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_certificate_digest_invalid_buf_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate_digest_response *resp =
		(struct cerberus_protocol_get_certificate_digest_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_DIGEST;

	resp->capabilities = 1;
	resp->num_digests = 3;
	request.length = sizeof (struct cerberus_protocol_get_certificate_digest_response) +
		(3 * SHA256_HASH_LENGTH) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, true, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_get_certificate_digest_response) +
		(3 * SHA256_HASH_LENGTH) - 1;
	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, true, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_certificate (CuTest *test,
	struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate_response *resp =
		(struct cerberus_protocol_get_certificate_response*) data;
	struct cerberus_protocol_get_certificate_digest *req =
		(struct cerberus_protocol_get_certificate_digest*) data;
	uint8_t expected[32];
	int status;

	memset (expected, 0x55, sizeof (expected));

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_CERTIFICATE;

	resp->slot_num = 0;
	resp->cert_num = 0;
	memcpy (&request.data[sizeof (*resp)], expected, sizeof (expected));
	request.length = sizeof (struct cerberus_protocol_get_certificate_response) + sizeof (expected);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock, master_attestation->base.store_certificate,
		master_attestation, 0, MOCK_ARG (MCTP_PROTOCOL_BMC_EID), MOCK_ARG (0), MOCK_ARG (0),
		MOCK_ARG_PTR_CONTAINS_TMP (expected, sizeof (expected)), MOCK_ARG (sizeof (expected)));
	CuAssertIntEquals (test, 0, status);

	request.new_request = false;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_certificate_digest),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_DIGEST, req->header.command);
	CuAssertIntEquals (test, 0, req->digest.slot_num);
	CuAssertIntEquals (test, ATTESTATION_KEY_EXCHANGE_NONE, req->digest.key_alg);
	CuAssertIntEquals (test, true, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_certificate_invalid_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate_response *resp =
		(struct cerberus_protocol_get_certificate_response*) data;
	uint8_t expected[32];
	int status;

	memset (expected, 0x55, sizeof (expected));

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_CERTIFICATE;

	resp->slot_num = 0;
	resp->cert_num = 0;
	memcpy (&request.data[sizeof (*resp)], expected, sizeof (expected));
	request.length = sizeof (struct cerberus_protocol_get_certificate_response) - 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_certificate_store_fail (
	CuTest *test, struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_certificate_response *resp =
		(struct cerberus_protocol_get_certificate_response*) data;
	uint8_t expected[32];
	int status;

	memset (expected, 0x55, sizeof (expected));

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_GET_CERTIFICATE;

	resp->slot_num = 0;
	resp->cert_num = 0;
	memcpy (&request.data[sizeof (*resp)], expected, sizeof (expected));
	request.length = sizeof (struct cerberus_protocol_get_certificate_response) + sizeof (expected);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock, master_attestation->base.store_certificate,
		master_attestation, ATTESTATION_NO_MEMORY, MOCK_ARG (MCTP_PROTOCOL_BMC_EID), MOCK_ARG (0),
		MOCK_ARG (0), MOCK_ARG_PTR_CONTAINS_TMP (expected, sizeof (expected)),
		MOCK_ARG (sizeof (expected)));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, ATTESTATION_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_challenge_response (CuTest *test,
	struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_challenge_response *resp =
		(struct cerberus_protocol_challenge_response*) data;
	struct attestation_response *response;
	uint8_t response_buf[136] = {0};
	int status;

	response = (struct attestation_response*) response_buf;

	response->slot_num = 0;
	response->slot_mask = 1;
	response->min_protocol_version = 1;
	response->max_protocol_version = 1;
	response->nonce[0] = 0xAA;
	response->nonce[31] = 0xBB;
	response->num_digests = 2;
	response->digests_size = SHA256_HASH_LENGTH;

	response_buf[sizeof (struct attestation_response)] = 0xCC;
	response_buf[sizeof (struct attestation_response) + 31] = 0xDD;
	response_buf[sizeof (struct attestation_response) + 32] = 0xEE;
	response_buf[sizeof (struct attestation_response) + 95] = 0xFF;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE;

	memcpy (&request.data[CERBERUS_PROTOCOL_MIN_MSG_LEN], response_buf, sizeof (response_buf));
	request.length = CERBERUS_PROTOCOL_MIN_MSG_LEN + sizeof (response_buf);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock,
		master_attestation->base.process_challenge_response, master_attestation, 0,
		MOCK_ARG_PTR_CONTAINS_TMP (response_buf, sizeof (response_buf)),
		MOCK_ARG (sizeof (response_buf)), MOCK_ARG (MCTP_PROTOCOL_BMC_EID));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, true, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_process_challenge_response_fail (
	CuTest *test, struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_challenge_response *resp =
		(struct cerberus_protocol_challenge_response*) data;
	struct attestation_response *response;
	uint8_t response_buf[136] = {0};
	int status;

	response = (struct attestation_response*) response_buf;

	response->slot_num = 0;
	response->slot_mask = 1;
	response->min_protocol_version = 1;
	response->max_protocol_version = 1;
	response->nonce[0] = 0xAA;
	response->nonce[31] = 0xBB;
	response->num_digests = 2;
	response->digests_size = SHA256_HASH_LENGTH;

	response_buf[sizeof (struct attestation_response)] = 0xCC;
	response_buf[sizeof (struct attestation_response) + 31] = 0xDD;
	response_buf[sizeof (struct attestation_response) + 32] = 0xEE;
	response_buf[sizeof (struct attestation_response) + 95] = 0xFF;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	resp->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	resp->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	resp->header.command = CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE;

	memcpy (&request.data[CERBERUS_PROTOCOL_MIN_MSG_LEN], response_buf, sizeof (response_buf));
	request.length = CERBERUS_PROTOCOL_MIN_MSG_LEN + sizeof (response_buf);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&master_attestation->mock,
		master_attestation->base.process_challenge_response, master_attestation,
		ATTESTATION_NO_MEMORY, MOCK_ARG_PTR_CONTAINS_TMP (response_buf, sizeof (response_buf)),
		MOCK_ARG (sizeof (response_buf)), MOCK_ARG (MCTP_PROTOCOL_BMC_EID));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = false;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, ATTESTATION_NO_MEMORY, status);
	CuAssertIntEquals (test, true, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_issue_get_device_capabilities (CuTest *test,
	struct cmd_interface *cmd)
{
	struct device_manager_capabilities expected;
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	struct cerberus_protocol_device_capabilities *req =
		(struct cerberus_protocol_device_capabilities*) buf;
	int status;

	memset (&expected, 0, sizeof (expected));
	expected.max_message_size = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	expected.max_packet_size = MCTP_PROTOCOL_MAX_TRANSMISSION_UNIT;
	expected.security_mode = DEVICE_MANAGER_SECURITY_AUTHENTICATION;
	expected.bus_role = DEVICE_MANAGER_SLAVE_BUS_ROLE;
	expected.hierarchy_role = DEVICE_MANAGER_AC_ROT_MODE;

	memset (buf, 0x55, sizeof (buf));

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_DEVICE_CAPABILITIES, NULL, buf,
		sizeof (buf));
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_device_capabilities), status);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_DEVICE_CAPABILITIES, req->header.command);

	status = testing_validate_array ((uint8_t*) &expected, (uint8_t*) &req->capabilities,
		sizeof (expected));
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_issue_get_device_capabilities_buf_too_small (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t buf[sizeof (struct cerberus_protocol_device_capabilities) - 1];
	int status;

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_DEVICE_CAPABILITIES, NULL, buf,
		sizeof (buf));
	CuAssertIntEquals (test, CMD_HANDLER_BUF_TOO_SMALL, status);
}

void cerberus_protocol_master_commands_testing_issue_get_certificate_digest (CuTest *test,
	struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	struct cerberus_protocol_get_certificate_digest *req =
		(struct cerberus_protocol_get_certificate_digest*) buf;
	int status;

	memset (buf, 0x55, sizeof (buf));

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_DIGEST, NULL, buf, sizeof (buf));
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_certificate_digest), status);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_DIGEST, req->header.command);
	CuAssertIntEquals (test, 0, req->digest.slot_num);
	CuAssertIntEquals (test, ATTESTATION_KEY_EXCHANGE_NONE, req->digest.key_alg);
}

void cerberus_protocol_master_commands_testing_issue_get_certificate_digest_buf_too_small (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t buf[sizeof (struct cerberus_protocol_get_certificate_digest) - 1];
	int status;

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_DIGEST, NULL, buf, sizeof (buf));
	CuAssertIntEquals (test, CMD_HANDLER_BUF_TOO_SMALL, status);
}

void cerberus_protocol_master_commands_testing_issue_get_certificate (CuTest *test,
	struct cmd_interface *cmd)
{
	struct cerberus_protocol_cert_req_params params = {0};
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	struct cerberus_protocol_get_certificate *req = (struct cerberus_protocol_get_certificate*) buf;
	int status;

	params.slot_num = 1;
	params.cert_num = 2;

	memset (buf, 0x55, sizeof (buf));

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_CERTIFICATE, &params, buf,
		sizeof (buf));
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_certificate), status);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CERTIFICATE, req->header.command);
	CuAssertIntEquals (test, 1, req->certificate.slot_num);
	CuAssertIntEquals (test, 2, req->certificate.cert_num);
	CuAssertIntEquals (test, 0, req->certificate.offset);
	CuAssertIntEquals (test, 0, req->certificate.length);
}

void cerberus_protocol_master_commands_testing_issue_get_certificate_buf_too_small (CuTest *test,
	struct cmd_interface *cmd)
{
	struct cerberus_protocol_cert_req_params params = {0};
	uint8_t buf[sizeof (struct cerberus_protocol_get_certificate) - 1];
	int status;

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_CERTIFICATE, &params, buf,
		sizeof (buf));
	CuAssertIntEquals (test, CMD_HANDLER_BUF_TOO_SMALL, status);
}

void cerberus_protocol_master_commands_testing_issue_get_certificate_null (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	int status;

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_GET_CERTIFICATE, NULL, buf,
		sizeof (buf));
	CuAssertIntEquals (test, CMD_HANDLER_INVALID_ARGUMENT, status);
}

void cerberus_protocol_master_commands_testing_issue_challenge (CuTest *test,
	struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	struct cerberus_protocol_challenge_req_params params = {0};
	struct attestation_challenge challenge = {0};
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	struct cerberus_protocol_challenge *req = (struct cerberus_protocol_challenge*) buf;
	int status;

	params.eid = 2;
	params.slot_num = 3;

	challenge.slot_num = 3;
	challenge.reserved = 0;
	challenge.nonce[0] = 0xAA;
	challenge.nonce[31] = 0xBB;

	status = mock_expect (&master_attestation->mock, master_attestation->base.issue_challenge,
		master_attestation, sizeof (struct attestation_challenge), MOCK_ARG (2), MOCK_ARG (3),
		MOCK_ARG (&buf[CERBERUS_PROTOCOL_MIN_MSG_LEN]),
		MOCK_ARG (sizeof (buf) - CERBERUS_PROTOCOL_MIN_MSG_LEN));
	status |= mock_expect_output (&master_attestation->mock, 2, &challenge,
		sizeof (struct attestation_challenge), -1);

	CuAssertIntEquals (test, 0, status);

	memset (buf, 0x55, sizeof (buf));

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE, &params, buf,
		sizeof (buf));
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_challenge), status);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, req->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0, req->header.reserved1);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE, req->header.command);
	CuAssertIntEquals (test, 3, req->challenge.slot_num);
	CuAssertIntEquals (test, 0, req->challenge.reserved);

	status = testing_validate_array (challenge.nonce, req->challenge.nonce, ATTESTATION_NONCE_LEN);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_issue_challenge_fail (CuTest *test,
	struct cmd_interface *cmd, struct attestation_master_mock *master_attestation)
{
	struct cerberus_protocol_challenge_req_params params = {0};
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	int status;

	params.eid = 2;
	params.slot_num = 3;

	status = mock_expect (&master_attestation->mock, master_attestation->base.issue_challenge,
		master_attestation, ATTESTATION_NO_MEMORY, MOCK_ARG (2), MOCK_ARG (3),
		MOCK_ARG (&buf[CERBERUS_PROTOCOL_MIN_MSG_LEN]),
		MOCK_ARG (sizeof (buf) - CERBERUS_PROTOCOL_MIN_MSG_LEN));

	CuAssertIntEquals (test, 0, status);

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE, &params, buf,
		sizeof (buf));
	CuAssertIntEquals (test, ATTESTATION_NO_MEMORY, status);
}

void cerberus_protocol_master_commands_testing_issue_challenge_null (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];
	int status;

	status = cmd->issue_request (cmd, CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE, NULL, buf,
		sizeof (buf));
	CuAssertIntEquals (test, CMD_HANDLER_INVALID_ARGUMENT, status);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_init (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_cfm_update *req =
		(struct cerberus_protocol_prepare_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_CFM_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.prepare_manifest, cfm, 0, MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_init_invalid_len (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_cfm_update *req =
		(struct cerberus_protocol_prepare_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_CFM_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_cfm_update) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_prepare_cfm_update) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_init_no_cfm_manager (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_cfm_update *req =
		(struct cerberus_protocol_prepare_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_CFM_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_COMMAND, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_init_fail (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_cfm_update *req =
		(struct cerberus_protocol_prepare_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_CFM_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.prepare_manifest, cfm, MANIFEST_NO_MEMORY,
		MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, MANIFEST_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_cfm_update *req = (struct cerberus_protocol_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_CFM_UPDATE;

	req->payload = 0xAA;
	request.length = sizeof (struct cerberus_protocol_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.store_manifest, cfm, 0,
		MOCK_ARG_PTR_CONTAINS_TMP (&req->payload, 1), MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_no_data (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_cfm_update *req = (struct cerberus_protocol_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_CFM_UPDATE;

	request.length = sizeof (struct cerberus_protocol_cfm_update) - 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_no_cfm_manager (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_cfm_update *req = (struct cerberus_protocol_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_CFM_UPDATE;

	req->payload = 0xAA;
	request.length = sizeof (struct cerberus_protocol_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_COMMAND, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_fail (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_cfm_update *req = (struct cerberus_protocol_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_CFM_UPDATE;

	req->payload = 0xAA;
	request.length = sizeof (struct cerberus_protocol_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.store_manifest, cfm, CFM_NO_MEMORY,
		MOCK_ARG_PTR_CONTAINS_TMP (&req->payload, 1), MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CFM_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_complete (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_cfm_update *req =
		(struct cerberus_protocol_complete_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_CFM_UPDATE;

	req->activation = 0;
	request.length = sizeof (struct cerberus_protocol_complete_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.finish_manifest, cfm, 0, MOCK_ARG (false));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_complete_immediate (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_cfm_update *req =
		(struct cerberus_protocol_complete_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_CFM_UPDATE;

	req->activation = 1;
	request.length = sizeof (struct cerberus_protocol_complete_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.finish_manifest, cfm, 0, MOCK_ARG (true));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_complete_invalid_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_cfm_update *req =
		(struct cerberus_protocol_complete_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_CFM_UPDATE;

	req->activation = 0;
	request.length = sizeof (struct cerberus_protocol_complete_cfm_update) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_complete_cfm_update) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_complete_no_cfm_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_cfm_update *req =
		(struct cerberus_protocol_complete_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_CFM_UPDATE;

	req->activation = 0;
	request.length = sizeof (struct cerberus_protocol_complete_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_COMMAND, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_cfm_update_complete_fail (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_cfm_update *req =
		(struct cerberus_protocol_complete_cfm_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_CFM_UPDATE;

	req->activation = 0;
	request.length = sizeof (struct cerberus_protocol_complete_cfm_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.finish_manifest, cfm, MANIFEST_NO_MEMORY,
		MOCK_ARG (false));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, MANIFEST_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_region0 (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_version_response *resp =
		(struct cerberus_protocol_get_cfm_id_version_response*) data;
	uint32_t cfm_id = 0xABCD;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, cfm_id, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_region1 (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_version_response *resp =
		(struct cerberus_protocol_get_cfm_id_version_response*) data;
	uint32_t cfm_id = 0xABCD;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 1;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_pending_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, cfm_id, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_no_id_type (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_version_response *resp =
		(struct cerberus_protocol_get_cfm_id_version_response*) data;
	uint32_t cfm_id = 0xABCD;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id) - sizeof (req->id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, cfm_id, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_invalid_len (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_get_cfm_id) - sizeof (req->id) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_invalid_region (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 2;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_fail (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, CFM_NO_MEMORY,
		MOCK_ARG_NOT_NULL);

	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CFM_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_no_cfm (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_version_response *resp =
		(struct cerberus_protocol_get_cfm_id_version_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) NULL);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (NULL));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_no_cfm_manager (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_version_response *resp =
		(struct cerberus_protocol_get_cfm_id_version_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, 0, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_invalid_id (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 2;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_platform_region0 (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_platform_response *resp =
		(struct cerberus_protocol_get_cfm_id_platform_response*) data;
	size_t id_length = CFM_PLATFORM_ID_LEN + 1;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG - 1;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_platform_id, &cfm_mock, 0,
		MOCK_ARG_PTR_PTR_NOT_NULL, MOCK_ARG (max));
	status |= mock_expect_output_ptr (&cfm_mock.mock, 0, CFM_PLATFORM_ID, id_length, 1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test,
		cerberus_protocol_get_cfm_id_platform_response_length (id_length), request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertStrEquals (test, CFM_PLATFORM_ID, (char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_platform_region1 (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_platform_response *resp =
		(struct cerberus_protocol_get_cfm_id_platform_response*) data;
	size_t id_length = CFM_PLATFORM_ID_LEN + 1;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG - 1;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 1;
	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_pending_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_platform_id, &cfm_mock, 0,
		MOCK_ARG_PTR_PTR_NOT_NULL, MOCK_ARG (max));
	status |= mock_expect_output_ptr (&cfm_mock.mock, 0, CFM_PLATFORM_ID, id_length, -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test,
		cerberus_protocol_get_cfm_id_platform_response_length (id_length), request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertStrEquals (test, CFM_PLATFORM_ID, (char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_platform_no_cfm (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_platform_response *resp =
		(struct cerberus_protocol_get_cfm_id_platform_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) NULL);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (NULL));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, cerberus_protocol_get_cfm_id_platform_response_length (1),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertStrEquals (test, "", (char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_platform_no_cfm_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	struct cerberus_protocol_get_cfm_id_platform_response *resp =
		(struct cerberus_protocol_get_cfm_id_platform_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_id_platform_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertStrEquals (test, "", (char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_id_platform_fail (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_id *req = (struct cerberus_protocol_get_cfm_id*) data;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG - 1;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_ID;

	req->region = 0;
	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_cfm_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager,
		0, MOCK_ARG (&cfm_mock.base));

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_platform_id, &cfm_mock,
		CFM_NO_MEMORY, MOCK_ARG_PTR_PTR_NOT_NULL, MOCK_ARG (max));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CFM_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_region0 (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	struct cfm_component_ids ids_list= {0};
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t ids[100];
	uint32_t cfm_id = 0xAABBCCDD;
	uint32_t offset = 0;
	int status;
	int i;

	for (i = 0; i < 100; ++i) {
		ids[i] = i;
	}

	ids_list.ids = ids;
	ids_list.count = 100;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.get_supported_component_ids, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &ids_list, sizeof (ids_list), -1);
	status |= mock_expect_save_arg (&cfm_mock.mock, 0, 0);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.free_component_ids, &cfm_mock, 0,
		MOCK_ARG_SAVED_ARG (0));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test,
		sizeof (struct cerberus_protocol_get_cfm_component_ids_response) + sizeof (ids),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, 0xAABBCCDD, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = testing_validate_array ((uint8_t*) ids, cerberus_protocol_cfm_component_ids (resp),
		sizeof (ids));
	CuAssertIntEquals (test, 0, status);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_region1 (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	struct cfm_component_ids ids_list= {0};
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t ids[100];
	uint32_t cfm_id = 0xAABBCCDD;
	uint32_t offset = 0;
	int status;
	int i;

	for (i = 0; i < 100; ++i) {
		ids[i] = i;
	}

	ids_list.ids = ids;
	ids_list.count = 100;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 1;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_pending_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.get_supported_component_ids, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &ids_list, sizeof (ids_list), -1);
	status |= mock_expect_save_arg (&cfm_mock.mock, 0, 0);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.free_component_ids, &cfm_mock, 0,
		MOCK_ARG_SAVED_ARG (0));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test,
		sizeof (struct cerberus_protocol_get_cfm_component_ids_response) + sizeof (ids),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, 0xAABBCCDD, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = testing_validate_array ((uint8_t*) ids, cerberus_protocol_cfm_component_ids (resp),
		sizeof (ids));
	CuAssertIntEquals (test, 0, status);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_nonzero_offset (
	CuTest *test, struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	struct cfm_component_ids ids_list= {0};
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t ids[100];
	uint32_t cfm_id = 0xAABBCCDD;
	uint32_t offset = sizeof (ids) / 2;
	int status;
	int i;

	for (i = 0; i < 100; ++i) {
		ids[i] = i;
	}

	ids_list.ids = ids;
	ids_list.count = 100;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.get_supported_component_ids, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &ids_list, sizeof (ids_list), -1);
	status |= mock_expect_save_arg (&cfm_mock.mock, 0, 0);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.free_component_ids, &cfm_mock, 0,
		MOCK_ARG_SAVED_ARG (0));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test,
		sizeof (struct cerberus_protocol_get_cfm_component_ids_response) + (sizeof (ids) / 2),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, 0xAABBCCDD, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = testing_validate_array ((uint8_t*) &ids[50],
		cerberus_protocol_cfm_component_ids (resp), sizeof (ids) / 2);
	CuAssertIntEquals (test, 0, status);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_limited_response (
	CuTest *test, struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	struct cfm_component_ids ids_list= {0};
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t ids[100];
	uint32_t cfm_id = 0xAABBCCDD;
	uint32_t offset = 0;
	int status;
	int i;
	int max = sizeof (ids) - 10 - sizeof (struct cerberus_protocol_get_cfm_component_ids_response);

	for (i = 0; i < 100; ++i) {
		ids[i] = i;
	}

	ids_list.ids = ids;
	ids_list.count = 100;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = sizeof (ids) - 10;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.get_supported_component_ids, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &ids_list, sizeof (ids_list), -1);
	status |= mock_expect_save_arg (&cfm_mock.mock, 0, 0);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.free_component_ids, &cfm_mock, 0,
		MOCK_ARG_SAVED_ARG (0));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_component_ids_response) + max,
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, 0xAABBCCDD, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = testing_validate_array ((uint8_t*) ids, cerberus_protocol_cfm_component_ids (resp),
		max);
	CuAssertIntEquals (test, 0, status);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_no_cfm_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_component_ids_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, 0, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_no_active_cfm (
	CuTest *test, struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) NULL);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (NULL));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_component_ids_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_no_pending_cfm (
	CuTest *test, struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 1;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_pending_cfm, cfm_manager,
		(intptr_t) NULL);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (NULL));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_component_ids_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_fail_id (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, CFM_NO_MEMORY,
		MOCK_ARG_NOT_NULL);

	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CFM_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_fail (CuTest *test,
	struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	uint32_t cfm_id = 0xAABBCCDD;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.get_supported_component_ids, &cfm_mock,
		CFM_NO_MEMORY, MOCK_ARG_NOT_NULL);

	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CFM_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_invalid_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_invalid_region (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	uint32_t offset = 0;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 2;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_component_ids_invalid_offset (
	CuTest *test, struct cmd_interface *cmd, struct cfm_manager_mock *cfm_manager)
{
	struct cfm_mock cfm_mock;
	struct cfm_component_ids ids_list= {0};
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_cfm_component_ids *req =
		(struct cerberus_protocol_get_cfm_component_ids*) data;
	struct cerberus_protocol_get_cfm_component_ids_response *resp =
		(struct cerberus_protocol_get_cfm_component_ids_response*) data;
	uint32_t ids[100];
	uint32_t cfm_id = 0xAABBCCDD;
	uint32_t offset = sizeof (ids);
	int status;
	int i;

	for (i = 0; i < 100; ++i) {
		ids[i] = i;
	}

	ids_list.ids = ids;
	ids_list.count = 100;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS;

	req->region = 0;
	req->offset = offset;
	request.length = sizeof (struct cerberus_protocol_get_cfm_component_ids);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = cfm_mock_init (&cfm_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_manager->mock, cfm_manager->base.get_active_cfm, cfm_manager,
		(intptr_t) &cfm_mock.base);
	status |= mock_expect (&cfm_manager->mock, cfm_manager->base.free_cfm, cfm_manager, 0,
		MOCK_ARG (&cfm_mock.base));

	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&cfm_mock.mock, cfm_mock.base.base.get_id, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &cfm_id, sizeof (cfm_id), -1);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.get_supported_component_ids, &cfm_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&cfm_mock.mock, 0, &ids_list, sizeof (ids_list), -1);
	status |= mock_expect_save_arg (&cfm_mock.mock, 0, 0);

	status |= mock_expect (&cfm_mock.mock, cfm_mock.base.free_component_ids, &cfm_mock, 0,
		MOCK_ARG_SAVED_ARG (0));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_cfm_component_ids_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, 0xAABBCCDD, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = cfm_mock_validate_and_release (&cfm_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	struct pcd_mock pcd_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_version_response *resp =
		(struct cerberus_protocol_get_pcd_id_version_response*) data;
	uint32_t pcd_id = 0xABCD;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = pcd_mock_init (&pcd_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) &pcd_mock.base);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (&pcd_mock.base));

	status |= mock_expect (&pcd_mock.mock, pcd_mock.base.base.get_id, &pcd_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&pcd_mock.mock, 0, &pcd_id, sizeof (pcd_id), -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_pcd_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, pcd_id, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = pcd_mock_validate_and_release (&pcd_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_no_id_type (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	struct pcd_mock pcd_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_version_response *resp =
		(struct cerberus_protocol_get_pcd_id_version_response*) data;
	uint32_t pcd_id = 0xABCD;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	request.length = sizeof (struct cerberus_protocol_get_pcd_id) - sizeof (req->id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = pcd_mock_init (&pcd_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) &pcd_mock.base);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (&pcd_mock.base));

	status |= mock_expect (&pcd_mock.mock, pcd_mock.base.base.get_id, &pcd_mock, 0,
		MOCK_ARG_NOT_NULL);
	status |= mock_expect_output (&pcd_mock.mock, 0, &pcd_id, sizeof (pcd_id), -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_pcd_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertIntEquals (test, pcd_id, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = pcd_mock_validate_and_release (&pcd_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_no_pcd (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_version_response *resp =
		(struct cerberus_protocol_get_pcd_id_version_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) NULL);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (NULL));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_pcd_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_no_pcd_manager (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_version_response *resp =
		(struct cerberus_protocol_get_pcd_id_version_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_pcd_id_version_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertIntEquals (test, 0, resp->version);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_invalid_len (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_fail (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	struct pcd_mock pcd_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 0;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = pcd_mock_init (&pcd_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) &pcd_mock.base);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (&pcd_mock.base));

	status |= mock_expect (&pcd_mock.mock, pcd_mock.base.base.get_id, &pcd_mock, PCD_NO_MEMORY,
		MOCK_ARG_NOT_NULL);

	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, PCD_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = pcd_mock_validate_and_release (&pcd_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_invalid_id (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 2;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_platform (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	struct pcd_mock pcd_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_platform_response *resp =
		(struct cerberus_protocol_get_pcd_id_platform_response*) data;
	size_t id_length = PCD_TESTING.manifest.plat_id_str_len + 1;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG - 1;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = pcd_mock_init (&pcd_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) &pcd_mock.base);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (&pcd_mock.base));

	status |= mock_expect (&pcd_mock.mock, pcd_mock.base.base.get_platform_id, &pcd_mock, 0,
		MOCK_ARG_PTR_PTR_NOT_NULL, MOCK_ARG (max));
	status |= mock_expect_output_ptr (&pcd_mock.mock, 0, PCD_TESTING.manifest.plat_id_str, 
		id_length, -1);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test,
		cerberus_protocol_get_pcd_id_platform_response_length (id_length), request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 1, resp->valid);
	CuAssertStrEquals (test, (const char*) PCD_TESTING.manifest.plat_id_str, 
		(const char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = pcd_mock_validate_and_release (&pcd_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_platform_no_pcd (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_platform_response *resp =
		(struct cerberus_protocol_get_pcd_id_platform_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) NULL);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (NULL));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, cerberus_protocol_get_pcd_id_platform_response_length (1),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertStrEquals (test, "", (char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_platform_no_pcd_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	struct cerberus_protocol_get_pcd_id_platform_response *resp =
		(struct cerberus_protocol_get_pcd_id_platform_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_get_pcd_id_platform_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp->header.command);
	CuAssertIntEquals (test, 0, resp->valid);
	CuAssertStrEquals (test, "", (char*) &resp->platform);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_id_platform_fail (CuTest *test,
	struct cmd_interface *cmd, struct pcd_manager_mock *pcd_manager)
{
	struct pcd_mock pcd_mock;
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_get_pcd_id *req = (struct cerberus_protocol_get_pcd_id*) data;
	int max = CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG - 1;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_PCD_ID;

	req->id = 1;
	request.length = sizeof (struct cerberus_protocol_get_pcd_id);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = pcd_mock_init (&pcd_mock);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&pcd_manager->mock, pcd_manager->base.get_active_pcd, pcd_manager,
		(intptr_t) &pcd_mock.base);
	status |= mock_expect (&pcd_manager->mock, pcd_manager->base.free_pcd, pcd_manager, 0,
		MOCK_ARG (&pcd_mock.base));

	status |= mock_expect (&pcd_mock.mock, pcd_mock.base.base.get_platform_id, &pcd_mock,
		PCD_NO_MEMORY, MOCK_ARG_PTR_PTR_NOT_NULL, MOCK_ARG (max));

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, PCD_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	status = pcd_mock_validate_and_release (&pcd_mock);
	CuAssertIntEquals (test, 0, status);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_init (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_pcd_update *req =
		(struct cerberus_protocol_prepare_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_PCD_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.prepare_manifest, pcd, 0, MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_init_no_pcd_manager (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_pcd_update *req =
		(struct cerberus_protocol_prepare_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_PCD_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_COMMAND, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_init_invalid_len (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_pcd_update *req =
		(struct cerberus_protocol_prepare_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_PCD_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_pcd_update) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_prepare_pcd_update) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_init_fail (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_prepare_pcd_update *req =
		(struct cerberus_protocol_prepare_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_INIT_PCD_UPDATE;

	req->total_size = 1;
	request.length = sizeof (struct cerberus_protocol_prepare_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.prepare_manifest, pcd, MANIFEST_NO_MEMORY,
		MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, MANIFEST_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_pcd_update *req = (struct cerberus_protocol_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_PCD_UPDATE;

	req->payload = 0xAA;
	request.length = sizeof (struct cerberus_protocol_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.store_manifest, pcd, 0,
		MOCK_ARG_PTR_CONTAINS_TMP (&req->payload, 1), MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_no_data (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_pcd_update *req = (struct cerberus_protocol_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_PCD_UPDATE;

	request.length = sizeof (struct cerberus_protocol_pcd_update) - 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_no_pcd_manager (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_pcd_update *req = (struct cerberus_protocol_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_PCD_UPDATE;

	req->payload = 0xAA;
	request.length = sizeof (struct cerberus_protocol_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_COMMAND, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_fail (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_pcd_update *req = (struct cerberus_protocol_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_PCD_UPDATE;

	req->payload = 0xAA;
	request.length = sizeof (struct cerberus_protocol_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.store_manifest, pcd, PCD_NO_MEMORY,
		MOCK_ARG_PTR_CONTAINS_TMP (&req->payload, 1), MOCK_ARG (1));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, PCD_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_complete (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_pcd_update *req =
		(struct cerberus_protocol_complete_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_PCD_UPDATE;

	request.length = sizeof (struct cerberus_protocol_complete_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.finish_manifest, pcd, 0, MOCK_ARG (true));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0, request.length);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_complete_no_pcd_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_pcd_update *req =
		(struct cerberus_protocol_complete_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_PCD_UPDATE;

	request.length = sizeof (struct cerberus_protocol_complete_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_COMMAND, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_complete_invalid_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_pcd_update *req =
		(struct cerberus_protocol_complete_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_PCD_UPDATE;

	request.length = sizeof (struct cerberus_protocol_complete_pcd_update) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_pcd_update_complete_fail (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_complete_pcd_update *req =
		(struct cerberus_protocol_complete_pcd_update*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_COMPLETE_PCD_UPDATE;

	request.length = sizeof (struct cerberus_protocol_complete_pcd_update);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.finish_manifest, pcd, MANIFEST_NO_MEMORY,
		MOCK_ARG (true));
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, MANIFEST_NO_MEMORY, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_fw_update_status (CuTest *test,
	struct cmd_interface *cmd, struct firmware_update_control_mock *update)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&update->mock, update->base.get_status, update, update_status);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_fw_update_status_no_fw_update (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_update_status_port0 (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pfm_0)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	status = mock_expect (&pfm_0->mock, pfm_0->base.get_status, pfm_0, update_status);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_update_status_port1 (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pfm_1)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	status = mock_expect (&pfm_1->mock, pfm_1->base.get_status, pfm_1, update_status);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_update_status_port0_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_update_status_port1_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_update_status_invalid_port (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 2;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_update_status (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *cfm)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 2;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&cfm->mock, cfm->base.get_status, cfm, 0x11223344);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, 0x11223344, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_update_status_no_cfm_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 2;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_update_status (CuTest *test,
	struct cmd_interface *cmd, struct manifest_cmd_interface_mock *pcd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 3;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&pcd->mock, pcd->base.get_status, pcd, 0x11223344);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, 0x11223344, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_update_status_no_pcd_manager (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 3;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_status_port0 (
	CuTest *test, struct cmd_interface *cmd, struct host_processor_mock *host_0)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&host_0->mock, host_0->base.get_next_reset_verification_actions, host_0,
		HOST_PROCESSOR_ACTION_VERIFY_PFM_AND_UPDATE);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, HOST_PROCESSOR_ACTION_VERIFY_PFM_AND_UPDATE, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_status_port1 (
	CuTest *test, struct cmd_interface *cmd, struct host_processor_mock *host_1)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&host_1->mock, host_1->base.get_next_reset_verification_actions, host_1,
		HOST_PROCESSOR_ACTION_VERIFY_PFM_AND_UPDATE);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, HOST_PROCESSOR_ACTION_VERIFY_PFM_AND_UPDATE, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_status_port0_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_status_port1_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_status_invalid_port (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 2;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_status_fail (
	CuTest *test, struct cmd_interface *cmd, struct host_processor_mock *host_0)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&host_0->mock, host_0->base.get_next_reset_verification_actions, host_0,
		HOST_PROCESSOR_NEXT_ACTIONS_FAILED);
	CuAssertIntEquals (test, 0, status);

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, HOST_PROCESSOR_NEXT_ACTIONS_FAILED, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_update_status_port0 (
	CuTest *test, struct cmd_interface *cmd, struct recovery_image_cmd_interface_mock *recovery_0)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	status = mock_expect (&recovery_0->mock, recovery_0->base.get_status, recovery_0,
		update_status);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_update_status_port1 (
	CuTest *test, struct cmd_interface *cmd, struct recovery_image_cmd_interface_mock *recovery_1)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	status = mock_expect (&recovery_1->mock, recovery_1->base.get_status, recovery_1,
		update_status);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_update_status_port0_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_update_status_port1_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_update_status_bad_port_index (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 2;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;
	request.target_eid = MCTP_PROTOCOL_BMC_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_reset_config_status (CuTest *test,
	struct cmd_interface *cmd, struct cmd_background_mock *background)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	struct cerberus_protocol_update_status_response *resp =
		(struct cerberus_protocol_update_status_response*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 6;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&background->mock, background->base.get_config_reset_status, background,
		0x00BB11AA);
	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, 0x00BB11AA, resp->update_status);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_reset_config_status_unsupported (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 6;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_update_status_invalid_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 0;
	request.length = sizeof (struct cerberus_protocol_update_status) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_update_status) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_update_status_invalid_type (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_update_status *req = (struct cerberus_protocol_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_UPDATE_STATUS;

	req->update_type = 7;
	request.length = sizeof (struct cerberus_protocol_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_fw_ext_update_status (CuTest *test,
	struct cmd_interface *cmd, struct firmware_update_control_mock *update)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	struct cerberus_protocol_extended_update_status_response *resp =
		(struct cerberus_protocol_extended_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int remaining_len = 0xAABBCCAA;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = mock_expect (&update->mock, update->base.get_status, update, update_status);
	status |= mock_expect (&update->mock, update->base.get_remaining_len, update, remaining_len);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_extended_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, remaining_len, resp->remaining_len);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_fw_ext_update_status_no_fw_update (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_ext_update_status_port0 (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pfm_ext_update_status_port1 (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 1;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_cfm_ext_update_status (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 2;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_pcd_ext_update_status (CuTest *test,
	struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 3;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_ext_status_port0 (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_host_fw_reset_verification_ext_status_port1 (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 4;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_port0 (
	CuTest *test, struct cmd_interface *cmd, struct recovery_image_cmd_interface_mock *recovery_0,
	struct recovery_image_manager_mock *recovery_manager_0, struct flash_mock *flash)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	struct cerberus_protocol_extended_update_status_response *resp =
		(struct cerberus_protocol_extended_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int remaining_len = 100;
	struct flash_updater updater;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = flash_updater_init (&updater, &flash->base, 0x10000, 0x10000);
	CuAssertIntEquals (test, 0, status);

	updater.update_size = remaining_len;

	status = mock_expect (&recovery_0->mock, recovery_0->base.get_status, recovery_0,
		update_status);
	status |= mock_expect (&recovery_manager_0->mock,
		recovery_manager_0->base.get_flash_update_manager, recovery_manager_0,
		(intptr_t) &updater);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_extended_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, remaining_len, resp->remaining_len);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	flash_updater_release (&updater);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_port1 (
	CuTest *test, struct cmd_interface *cmd, struct recovery_image_cmd_interface_mock *recovery_1,
	struct recovery_image_manager_mock *recovery_manager_1, struct flash_mock *flash)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	struct cerberus_protocol_extended_update_status_response *resp =
		(struct cerberus_protocol_extended_update_status_response*) data;
	int update_status = 0x00BB11AA;
	int remaining_len = 100;
	struct flash_updater updater;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	status = flash_updater_init (&updater, &flash->base, 0x10000, 0x10000);
	CuAssertIntEquals (test, 0, status);

	updater.update_size = remaining_len;

	status = mock_expect (&recovery_1->mock, recovery_1->base.get_status, recovery_1,
		update_status);
	status |= mock_expect (&recovery_manager_1->mock,
		recovery_manager_1->base.get_flash_update_manager, recovery_manager_1,
		(intptr_t) &updater);

	CuAssertIntEquals (test, 0, status);

	request.new_request = true;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, sizeof (struct cerberus_protocol_extended_update_status_response),
		request.length);
	CuAssertIntEquals (test, MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF, resp->header.msg_type);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_MSFT_PCI_VID, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0, resp->header.reserved1);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS, resp->header.command);
	CuAssertIntEquals (test, update_status, resp->update_status);
	CuAssertIntEquals (test, remaining_len, resp->remaining_len);
	CuAssertIntEquals (test, false, request.new_request);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	flash_updater_release (&updater);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_port0_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_port0_cmd_intf_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_port1_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_port1_cmd_intf_null (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 1;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_recovery_image_ext_update_status_bad_port_index (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 5;
	req->port_id = 2;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_OUT_OF_RANGE, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_reset_config_ext_update_status (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 6;
	req->port_id = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_ext_update_status_invalid_len (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 0;
	request.length = sizeof (struct cerberus_protocol_extended_update_status) + 1;
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;


	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);

	request.length = sizeof (struct cerberus_protocol_extended_update_status) - 1;
	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_BAD_LENGTH, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}

void cerberus_protocol_master_commands_testing_process_get_ext_update_status_invalid_type (
	CuTest *test, struct cmd_interface *cmd)
{
	uint8_t data[MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	struct cmd_interface_request request;
	struct cerberus_protocol_extended_update_status *req =
		(struct cerberus_protocol_extended_update_status*) data;
	int status;

	memset (&request, 0, sizeof (request));
	memset (data, 0, sizeof (data));
	request.data = data;
	req->header.msg_type = MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF;
	req->header.pci_vendor_id = CERBERUS_PROTOCOL_MSFT_PCI_VID;
	req->header.command = CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS;

	req->update_type = 7;
	request.length = sizeof (struct cerberus_protocol_extended_update_status);
	request.max_response = MCTP_PROTOCOL_MAX_MESSAGE_BODY;
	request.source_eid = MCTP_PROTOCOL_BMC_EID;
	request.target_eid = MCTP_PROTOCOL_PA_ROT_CTRL_EID;

	request.crypto_timeout = true;
	status = cmd->process_request (cmd, &request);
	CuAssertIntEquals (test, CMD_HANDLER_UNSUPPORTED_INDEX, status);
	CuAssertIntEquals (test, false, request.crypto_timeout);
}


/*******************
 * Test cases
 *******************/

static void cerberus_protocol_master_commands_test_get_cfm_id_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x5e,
		0x01,0x02
	};
	uint8_t raw_buffer_resp_version[] = {
		0x7e,0x14,0x13,0x03,0x5e,
		0x03,0x04,0x05,0x06,0x07
	};
	uint8_t raw_buffer_resp_platform[] = {
		0x7e,0x14,0x13,0x03,0x5e,
		0x08,
		0x30,0x31,0x32,0x33,0x34,0x35,0x00
	};
	struct cerberus_protocol_get_cfm_id *req;
	struct cerberus_protocol_get_cfm_id_version_response *resp1;
	struct cerberus_protocol_get_cfm_id_platform_response *resp2;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req), sizeof (struct cerberus_protocol_get_cfm_id));
	CuAssertIntEquals (test, sizeof (raw_buffer_resp_version),
		sizeof (struct cerberus_protocol_get_cfm_id_version_response));

	req = (struct cerberus_protocol_get_cfm_id*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, req->header.command);

	CuAssertIntEquals (test, 0x01, req->region);
	CuAssertIntEquals (test, 0x02, req->id);

	resp1 = (struct cerberus_protocol_get_cfm_id_version_response*) raw_buffer_resp_version;
	CuAssertIntEquals (test, 0, resp1->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp1->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp1->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp1->header.rq);
	CuAssertIntEquals (test, 0, resp1->header.reserved2);
	CuAssertIntEquals (test, 0, resp1->header.crypt);
	CuAssertIntEquals (test, 0x03, resp1->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp1->header.command);

	CuAssertIntEquals (test, 0x03, resp1->valid);
	CuAssertIntEquals (test, 0x07060504, resp1->version);

	resp2 = (struct cerberus_protocol_get_cfm_id_platform_response*) raw_buffer_resp_platform;
	CuAssertIntEquals (test, 0, resp2->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp2->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp2->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp2->header.rq);
	CuAssertIntEquals (test, 0, resp2->header.reserved2);
	CuAssertIntEquals (test, 0, resp2->header.crypt);
	CuAssertIntEquals (test, 0x03, resp2->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_ID, resp2->header.command);

	CuAssertIntEquals (test, 0x08, resp2->valid);
	CuAssertStrEquals (test, "012345", (char*) &resp2->platform);
}

static void cerberus_protocol_master_commands_test_prepare_cfm_update_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x5f,
		0x01,0x02,0x03,0x04
	};
	struct cerberus_protocol_prepare_cfm_update *req;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_prepare_cfm_update));

	req = (struct cerberus_protocol_prepare_cfm_update*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_INIT_CFM_UPDATE, req->header.command);

	CuAssertIntEquals (test, 0x04030201, req->total_size);
}

static void cerberus_protocol_master_commands_test_cfm_update_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x60,
		0x01,0x02,0x03,0x04
	};
	struct cerberus_protocol_cfm_update *req;

	TEST_START;

	req = (struct cerberus_protocol_cfm_update*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_CFM_UPDATE, req->header.command);

	CuAssertPtrEquals (test, &raw_buffer_req[5], &req->payload);
}

static void cerberus_protocol_master_commands_test_complete_cfm_update_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x61,
		0x01
	};
	struct cerberus_protocol_complete_cfm_update *req;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_complete_cfm_update));

	req = (struct cerberus_protocol_complete_cfm_update*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_COMPLETE_CFM_UPDATE, req->header.command);

	CuAssertIntEquals (test, 0x01, req->activation);
}

static void cerberus_protocol_master_commands_test_get_cfm_component_ids_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x8d,
		0x01,0x02,0x03,0x04,0x05
	};
	uint8_t raw_buffer_resp[] = {
		0x7e,0x14,0x13,0x03,0x8d,
		0x03,0x04,0x05,0x06,0x07,
		0x30,0x31,0x32,0x33,0x34,0x35,0x00
	};
	struct cerberus_protocol_get_cfm_component_ids *req;
	struct cerberus_protocol_get_cfm_component_ids_response *resp;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_get_cfm_component_ids));

	req = (struct cerberus_protocol_get_cfm_component_ids*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		req->header.command);

	CuAssertIntEquals (test, 0x01, req->region);
	CuAssertIntEquals (test, 0x05040302, req->offset);

	resp = (struct cerberus_protocol_get_cfm_component_ids_response*) raw_buffer_resp;
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0x03, resp->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CFM_SUPPORTED_COMPONENT_IDS,
		resp->header.command);

	CuAssertIntEquals (test, 0x03, resp->valid);
	CuAssertIntEquals (test, 0x07060504, resp->version);
	CuAssertPtrEquals (test, &raw_buffer_resp[10], cerberus_protocol_cfm_component_ids (resp));
}

static void cerberus_protocol_master_commands_test_get_pcd_id_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x62,
		0x01
	};
	uint8_t raw_buffer_resp_version[] = {
		0x7e,0x14,0x13,0x03,0x62,
		0x03,0x04,0x05,0x06,0x07
	};
	uint8_t raw_buffer_resp_platform[] = {
		0x7e,0x14,0x13,0x03,0x62,
		0x08,
		0x30,0x31,0x32,0x33,0x34,0x35,0x00
	};
	struct cerberus_protocol_get_pcd_id *req;
	struct cerberus_protocol_get_pcd_id_version_response *resp1;
	struct cerberus_protocol_get_pcd_id_platform_response *resp2;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req), sizeof (struct cerberus_protocol_get_pcd_id));
	CuAssertIntEquals (test, sizeof (raw_buffer_resp_version),
		sizeof (struct cerberus_protocol_get_pcd_id_version_response));

	req = (struct cerberus_protocol_get_pcd_id*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, req->header.command);

	CuAssertIntEquals (test, 0x01, req->id);

	resp1 = (struct cerberus_protocol_get_pcd_id_version_response*) raw_buffer_resp_version;
	CuAssertIntEquals (test, 0, resp1->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp1->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp1->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp1->header.rq);
	CuAssertIntEquals (test, 0, resp1->header.reserved2);
	CuAssertIntEquals (test, 0, resp1->header.crypt);
	CuAssertIntEquals (test, 0x03, resp1->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp1->header.command);

	CuAssertIntEquals (test, 0x03, resp1->valid);
	CuAssertIntEquals (test, 0x07060504, resp1->version);

	resp2 = (struct cerberus_protocol_get_pcd_id_platform_response*) raw_buffer_resp_platform;
	CuAssertIntEquals (test, 0, resp2->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp2->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp2->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp2->header.rq);
	CuAssertIntEquals (test, 0, resp2->header.reserved2);
	CuAssertIntEquals (test, 0, resp2->header.crypt);
	CuAssertIntEquals (test, 0x03, resp2->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_PCD_ID, resp2->header.command);

	CuAssertIntEquals (test, 0x08, resp2->valid);
	CuAssertStrEquals (test, "012345", (char*) &resp2->platform);
}

static void cerberus_protocol_master_commands_test_prepare_pcd_update_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x63,
		0x01,0x02,0x03,0x04
	};
	struct cerberus_protocol_prepare_pcd_update *req;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_prepare_pcd_update));

	req = (struct cerberus_protocol_prepare_pcd_update*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_INIT_PCD_UPDATE, req->header.command);

	CuAssertIntEquals (test, 0x04030201, req->total_size);
}

static void cerberus_protocol_master_commands_test_pcd_update_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x64,
		0x01,0x02,0x03,0x04
	};
	struct cerberus_protocol_pcd_update *req;

	TEST_START;

	req = (struct cerberus_protocol_pcd_update*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_PCD_UPDATE, req->header.command);

	CuAssertPtrEquals (test, &raw_buffer_req[5], &req->payload);
}

static void cerberus_protocol_master_commands_test_complete_pcd_update_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x65
	};
	struct cerberus_protocol_complete_pcd_update *req;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_complete_pcd_update));

	req = (struct cerberus_protocol_complete_pcd_update*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_COMPLETE_PCD_UPDATE, req->header.command);
}

static void cerberus_protocol_master_commands_test_update_status_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x68,
		0x01,0x02
	};
	uint8_t raw_buffer_resp[] = {
		0x7e,0x14,0x13,0x03,0x68,
		0x03,0x04,0x05,0x06
	};
	struct cerberus_protocol_update_status *req;
	struct cerberus_protocol_update_status_response *resp;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_update_status));
	CuAssertIntEquals (test, sizeof (raw_buffer_resp),
		sizeof (struct cerberus_protocol_update_status_response));

	req = (struct cerberus_protocol_update_status*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, req->header.command);

	CuAssertIntEquals (test, 0x01, req->update_type);
	CuAssertIntEquals (test, 0x02, req->port_id);

	resp = (struct cerberus_protocol_update_status_response*) raw_buffer_resp;
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0x03, resp->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_UPDATE_STATUS, resp->header.command);

	CuAssertIntEquals (test, 0x06050403, resp->update_status);
}

static void cerberus_protocol_master_commands_test_extended_update_status_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x8e,
		0x01,0x02
	};
	uint8_t raw_buffer_resp[] = {
		0x7e,0x14,0x13,0x03,0x8e,
		0x03,0x04,0x05,0x06,0x7,0x08,0x09,0x0a
	};
	struct cerberus_protocol_extended_update_status *req;
	struct cerberus_protocol_extended_update_status_response *resp;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_extended_update_status));
	CuAssertIntEquals (test, sizeof (raw_buffer_resp),
		sizeof (struct cerberus_protocol_extended_update_status_response));

	req = (struct cerberus_protocol_extended_update_status*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS, req->header.command);

	CuAssertIntEquals (test, 0x01, req->update_type);
	CuAssertIntEquals (test, 0x02, req->port_id);

	resp = (struct cerberus_protocol_extended_update_status_response*) raw_buffer_resp;
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0x03, resp->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS, resp->header.command);

	CuAssertIntEquals (test, 0x06050403, resp->update_status);
	CuAssertIntEquals (test, 0x0a090807, resp->remaining_len);
}

static void cerberus_protocol_master_commands_test_get_configuration_ids_format (CuTest *test)
{
	uint8_t raw_buffer_req[] = {
		0x7e,0x14,0x13,0x03,0x70,
		0x88,0x69,0xde,0x57,0x9d,0xd0,0xe9,0x05,0xe0,0xa7,0x11,0x24,0x57,0x55,0x94,0xf5,
		0x0a,0x03,0xd3,0xd9,0xcd,0xf1,0x6e,0x9a,0x3f,0x9d,0x6c,0x60,0xc0,0x32,0x4b,0x54
	};
	uint8_t raw_buffer_resp[] = {
		0x7e,0x14,0x13,0x03,0x70,
		0xf1,0x3b,0x43,0x16,0x2c,0xe4,0x05,0x75,0x73,0xc5,0x54,0x10,0xad,0xd5,0xc5,0xc6,
		0x0e,0x9a,0x37,0xff,0x3e,0xa0,0x02,0x34,0xd6,0x41,0x80,0xfa,0x1a,0x0e,0x0a,0x04,
		0x02,0x01,
		0x11,0x11,0x11,0x11,0x22,0x22,0x22,0x22,0x33,0x33,0x33,0x33,0x44,0x44,0x44,0x44,
		0x30,0x31,0x00,0x32,0x33,0x00,0x34,0x35,0x00,0x36,0x37,0x00,
		0x30,0x46,0x02,0x21,0x00,0x86,0x1d,0x0e,0x39,0x20,0xdc,0xae,0x77,0xcc,0xb0,0x33,
		0x38,0xb7,0xd8,0x47,0xb9,0x7a,0x6b,0x65,0x3b,0xe2,0x72,0x52,0x8f,0x77,0x82,0x00,
		0x82,0x8f,0x6f,0xc5,0x9e,0x02,0x21,0x00,0xf8,0xf9,0x96,0xaf,0xd5,0xc5,0x50,0x16,
		0xa9,0x31,0x2d,0xad,0x1e,0xec,0x61,0x3a,0x80,0xe5,0x7a,0x1f,0xa0,0xc3,0x0c,0x35,
		0x41,0x00,0x96,0xcf,0x71,0x24,0x08,0x43
	};
	struct cerberus_protocol_get_configuration_ids *req;
	struct cerberus_protocol_get_configuration_ids_response *resp;

	TEST_START;

	CuAssertIntEquals (test, sizeof (raw_buffer_req),
		sizeof (struct cerberus_protocol_get_configuration_ids));

	req = (struct cerberus_protocol_get_configuration_ids*) raw_buffer_req;
	CuAssertIntEquals (test, 0, req->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, req->header.msg_type);
	CuAssertIntEquals (test, 0x1314, req->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, req->header.rq);
	CuAssertIntEquals (test, 0, req->header.reserved2);
	CuAssertIntEquals (test, 0, req->header.crypt);
	CuAssertIntEquals (test, 0x03, req->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CONFIG_ID, req->header.command);

	CuAssertPtrEquals (test, &raw_buffer_req[5], req->nonce);

	resp = (struct cerberus_protocol_get_configuration_ids_response*) raw_buffer_resp;
	CuAssertIntEquals (test, 0, resp->header.integrity_check);
	CuAssertIntEquals (test, 0x7e, resp->header.msg_type);
	CuAssertIntEquals (test, 0x1314, resp->header.pci_vendor_id);
	CuAssertIntEquals (test, 0, resp->header.rq);
	CuAssertIntEquals (test, 0, resp->header.reserved2);
	CuAssertIntEquals (test, 0, resp->header.crypt);
	CuAssertIntEquals (test, 0x03, resp->header.reserved1);
	CuAssertIntEquals (test, CERBERUS_PROTOCOL_GET_CONFIG_ID, resp->header.command);

	CuAssertPtrEquals (test, &raw_buffer_resp[5], resp->nonce);
	CuAssertIntEquals (test, 0x02, resp->pfm_count);
	CuAssertIntEquals (test, 0x01, resp->cfm_count);
	CuAssertIntEquals (test, 0x11111111, resp->version_id);
	CuAssertStrEquals (test, "01", (char*) cerberus_protocol_configuration_ids_get_platform_ids (resp));
}


CuSuite* get_cerberus_protocol_master_commands_suite ()
{
	CuSuite *suite = CuSuiteNew ();

	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_get_cfm_id_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_prepare_cfm_update_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_cfm_update_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_complete_cfm_update_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_get_cfm_component_ids_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_get_pcd_id_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_prepare_pcd_update_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_pcd_update_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_complete_pcd_update_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_update_status_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_extended_update_status_format);
	SUITE_ADD_TEST (suite, cerberus_protocol_master_commands_test_get_configuration_ids_format);

	return suite;
}
