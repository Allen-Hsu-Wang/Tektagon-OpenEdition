// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "cerberus_protocol_debug_commands.h"


#ifdef ENABLE_DEBUG_COMMANDS
/**
 * Process log fill packet
 *
 * @param background Command background instance to utilize
 * @param request Log fill request to process
 *
 * @return 0 if request processing completed successfully or an error code.
 */
int cerberus_protocol_debug_fill_log (struct cmd_background *background,
	struct cmd_interface_request *request)
{
	if (request->length != CERBERUS_PROTOCOL_MIN_MSG_LEN) {
		return CMD_HANDLER_BAD_LENGTH;
	}

	request->length = 0;

	return background->debug_log_fill (background);
}

/**
 * Process get device certificate packet
 *
 * @param device_mgr Device manager instance to utilize
 * @param request Get device certificate request to process
 *
 * @return 0 if request processing completed successfully or an error code.
 */
int cerberus_protocol_get_device_certificate (struct device_manager *device_mgr,
	struct cmd_interface_request *request)
{
	struct device_manager_cert_chain chain;
	uint8_t device_num;
	uint8_t cert_num;
	int status;

	if (request->length != (CERBERUS_PROTOCOL_MIN_MSG_LEN + 3)) {
		return CMD_HANDLER_BAD_LENGTH;
	}

	device_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN];
	cert_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN + 2];

	status = device_manager_get_device_cert_chain (device_mgr, device_num, &chain);
	if (status != 0) {
		return status;
	}

	if (chain.num_cert <= cert_num) {
		return DEVICE_MGR_INVALID_CERT_NUM;
	}

	memcpy (&request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN + 3], (uint8_t*) chain.cert[cert_num].cert,
		chain.cert[cert_num].length);

	request->length = CERBERUS_PROTOCOL_MIN_MSG_LEN + 3 + chain.cert[cert_num].length;
	return 0;
}

/**
 * Process get device certificate digest packet
 *
 * @param device_mgr Device manager instance to utilize
 * @param hash Hash engine to utilize
 * @param request Get device certificate digest request to process
 *
 * @return 0 if request processing completed successfully or an error code.
 */
int cerberus_protocol_get_device_cert_digest (struct device_manager *device_mgr,
	struct hash_engine *hash, struct cmd_interface_request *request)
{
	struct device_manager_cert_chain chain;
	uint8_t device_num;
	uint8_t cert_num;
	int status;

	if (request->length != (CERBERUS_PROTOCOL_MIN_MSG_LEN + 3)) {
		return CMD_HANDLER_BAD_LENGTH;
	}

	device_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN];
	cert_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN + 2];

	status = device_manager_get_device_cert_chain (device_mgr, device_num, &chain);
	if (status != 0) {
		return status;
	}

	if (chain.num_cert <= cert_num) {
		return DEVICE_MGR_INVALID_CERT_NUM;
	}

	status = hash->calculate_sha256 (hash, chain.cert[cert_num].cert, chain.cert[cert_num].length,
		&request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN + 3], SHA256_HASH_LENGTH);
	if (status != 0) {
		return status;
	}

	request->length = CERBERUS_PROTOCOL_MIN_MSG_LEN + 3 + SHA256_HASH_LENGTH;
	return 0;
}

/**
 * Process get device challenge packet
 *
 * @param device_mgr Device manager instance to utilize
 * @param attestation Attestation manager instance to utilize
 * @param hash Hash engine to utilize
 * @param request Get device challenge request to process
 *
 * @return 0 if request processing completed successfully or an error code.
 */
int cerberus_protocol_get_device_challenge (struct device_manager *device_mgr,
	struct attestation_master *attestation, struct hash_engine *hash,
	struct cmd_interface_request *request)
{
	uint8_t device_num;
	int status;

	if (request->length != (CERBERUS_PROTOCOL_MIN_MSG_LEN + 1)) {
		return CMD_HANDLER_BAD_LENGTH;
	}

	device_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN];

	status = device_manager_get_device_state (device_mgr, device_num);
	if (ROT_IS_ERROR (status)) {
		return status;
	}

	memcpy (&request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN + 1],
		attestation->challenge[device_num].nonce, ATTESTATION_NONCE_LEN);

	request->length = CERBERUS_PROTOCOL_MIN_MSG_LEN + 1 + ATTESTATION_NONCE_LEN;
	return 0;
}


/**
 * Process start attestation packet
 *
 * @param request Start attestation request to process
 *
 * @return 0 if request processing completed successfully or an error code.
 */
int cerberus_protocol_start_attestation (struct cmd_interface_request *request)
{
	int status = 0;
	uint8_t device_num;

	if (request->length != (CERBERUS_PROTOCOL_MIN_MSG_LEN + 1)) {
		return CMD_HANDLER_BAD_LENGTH;
	}

	device_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN];

	status = (device_num << 16) | ((uint16_t) ATTESTATION_START_TEST_ESCAPE_SEQ);

	return status;
}

/**
 * Process get attestation state packet
 *
 * @param device_mgr Device manager instance to utilize
 * @param request Attestation state request to process
 *
 * @return 0 if request processing completed successfully or an error code.
 */
int cerberus_protocol_get_attestation_state (struct device_manager *device_mgr,
	struct cmd_interface_request *request)
{
	uint8_t device_num;
	int status;

	if (request->length != (CERBERUS_PROTOCOL_MIN_MSG_LEN + 1)) {
		return CMD_HANDLER_BAD_LENGTH;
	}

	device_num = request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN];
	status = device_manager_get_device_state (device_mgr, device_num);
	if (ROT_IS_ERROR (status)) {
		return status;
	}

	request->data[CERBERUS_PROTOCOL_MIN_MSG_LEN] = (uint8_t) status;
	request->length = CERBERUS_PROTOCOL_MIN_MSG_LEN + sizeof (uint8_t);
	return 0;
}
#endif
