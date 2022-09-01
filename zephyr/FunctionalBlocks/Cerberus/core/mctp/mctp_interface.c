// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common/common_math.h"
#include "cmd_interface/cerberus_protocol.h"
#include "cmd_interface/cerberus_protocol_debug_commands.h"
#include "cmd_interface/cmd_interface.h"
#include "cmd_interface/cmd_channel.h"
#include "cmd_interface/cmd_interface_system.h"
#include "mctp_logging.h"
#include "mctp_protocol.h"
#include "mctp_interface_control.h"
#include "mctp_interface.h"


/**
 * MCTP interface initialization
 *
 * @param interface The MCTP interface to initialize.
 * @param cmd_interface The command interface to use for processing and generating messages.
 * @param device_mgr The device manager linked to command interface.
 * @param eid MCTP EID to listen to.
 * @param pci_vid PCI vendor ID.
 * @param protocol_version Supported protocol version.
 *
 * @return Initialization status, 0 if success or an error code.
 */
int mctp_interface_init (struct mctp_interface *interface, struct cmd_interface *cmd_interface,
	struct device_manager *device_mgr, uint8_t eid, uint16_t pci_vid, uint16_t protocol_version)
{
	if ((interface == NULL) || (cmd_interface == NULL) || (device_mgr == NULL)) {
		return MCTP_PROTOCOL_INVALID_ARGUMENT;
	}

	memset (interface, 0, sizeof (struct mctp_interface));

	interface->device_manager = device_mgr;
	interface->cmd_interface = cmd_interface;
	interface->eid = eid;
	interface->pci_vendor_id = pci_vid;
	interface->protocol_version = protocol_version;

	interface->req_buffer.data =
		&interface->msg_buffer[sizeof (interface->msg_buffer) - MCTP_PROTOCOL_MAX_MESSAGE_BODY];
	interface->resp_buffer.data = interface->msg_buffer;

	return 0;
}

/**
 * MCTP interface deinitialization
 *
 * @param interface The MCTP interface to deinitialize
 */
void mctp_interface_deinit (struct mctp_interface *interface)
{
	if (interface != NULL) {
		memset (interface, 0, sizeof (struct mctp_interface));
	}
}

/**
 * Assign a channel ID to the mctp interface
 *
 * @param interface The MCTP interface to assign the channel id.
 * @param channel_id The channel ID to associate with this interface.
 *
 * @return 0 if the channel was successfully initialized or an error code.
 */
int mctp_interface_set_channel_id (struct mctp_interface *interface, int channel_id)
{
	if (interface == NULL) {
		return MCTP_PROTOCOL_INVALID_ARGUMENT;
	}

	interface->channel_id = channel_id;

	return 0;
}

/**
 * Construct an MCTP packet for an error response.
 *
 * @param interface MCTP interface instance.
 * @param packets Output for the buffer containing the error message.
 * @param error_code Identifier for the error.
 * @param error_data Data for the error condition.
 * @param src_eid EID of the original message source.
 * @param dest_eid EID of the original message destination.
 * @param msg_tag Tag of the original message.
 * @param response_addr SMBUS address to respond to.
 * @param source_addr SMBUS address responding from.
 * @param cmd_set Command set to respond on.
 *
 * @return 0 if the packet was successfully constructed or an error code.
 */
static int mctp_interface_generate_error_packet (struct mctp_interface *interface,
	struct cmd_message **message, uint8_t error_code, uint32_t error_data, uint8_t src_eid,
	uint8_t dest_eid, uint8_t msg_tag, uint8_t response_addr, uint8_t source_addr, uint8_t cmd_set)
{
	int status;

	if (error_code != CERBERUS_PROTOCOL_NO_ERROR) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_MCTP,
			MCTP_LOGGING_CHANNEL, interface->channel_id, 0);
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_MCTP,
			MCTP_LOGGING_PROTOCOL_ERROR,
			(error_code << 24 | src_eid << 16 | dest_eid << 8 | msg_tag), error_data);
	}

	if (dest_eid != interface->eid) {
		return 0;
	}

	mctp_interface_reset_message_processing (interface);

	interface->req_buffer.max_response = MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT;
	status = interface->cmd_interface->generate_error_packet (interface->cmd_interface,
		&interface->req_buffer, error_code, error_data, cmd_set);
	if (ROT_IS_ERROR (status)) {
		return status;
	}

	if (interface->req_buffer.length > MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT) {
		return MCTP_PROTOCOL_MSG_TOO_LARGE;
	}

	status = mctp_protocol_construct (interface->req_buffer.data, interface->req_buffer.length,
		interface->resp_buffer.data, sizeof (interface->msg_buffer), source_addr, src_eid,
		dest_eid, true, true, 0, msg_tag, MCTP_PROTOCOL_TO_RESPONSE, response_addr,
		&interface->msg_type);
	if (ROT_IS_ERROR (status)) {
		return status;
	}

	interface->resp_buffer.msg_size = status;
	interface->resp_buffer.pkt_size = status;
	interface->resp_buffer.dest_addr = response_addr;

	*message = &interface->resp_buffer;
	return 0;
}

/**
 * MCTP interface message processing function
 *
 * @param interface MCTP interface instance
 * @param rx_packet The received packet to process
 * @param tx_message Output for a response message to send.  This pointer MUST NOT be freed by the
 * caller.
 *
 * @return Completion status, 0 if success or an error code.
 */
int mctp_interface_process_packet (struct mctp_interface *interface, struct cmd_packet *rx_packet,
	struct cmd_message **tx_message)
{
	struct cerberus_protocol_header *header;
	uint32_t msg1 = 0;
	uint32_t msg2 = 0;
	uint8_t i_byte;
	uint8_t *payload;
	uint8_t source_addr;
	uint8_t src_eid;
	uint8_t dest_eid;
	uint8_t msg_tag;
	uint8_t packet_seq;
	uint8_t crc;
	uint8_t i_packet;
	uint8_t tag_owner;
	uint8_t response_addr;
	uint8_t cmd_set = 0;
	size_t n_packets;
	size_t payload_len;
	size_t max_packet;
	bool som;
	bool eom;
	int i_buf;
	int status;

	if ((interface == NULL) || (rx_packet == NULL) || (tx_message == NULL)) {
		return MCTP_PROTOCOL_INVALID_ARGUMENT;
	}

	*tx_message = NULL;

	status = mctp_protocol_interpret (rx_packet->data, rx_packet->pkt_size, rx_packet->dest_addr,
		&source_addr, &som, &eom, &src_eid, &dest_eid, &payload, &payload_len, &msg_tag,
		&packet_seq, &crc, &interface->msg_type);

	response_addr = source_addr;

	if (status != 0) {
		msg2 = rx_packet->pkt_size << 24;
		for (i_byte = 0; i_byte < 7; ++i_byte) {
			if (i_byte < 4) {
				msg1 |= (rx_packet->data[i_byte] << (i_byte * 8));
			}
			else {
				msg2 |= (rx_packet->data[i_byte] << ((i_byte - 4) * 8));
			}
		}

		debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_MCTP,
			MCTP_LOGGING_CHANNEL, interface->channel_id, 0);
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_MCTP,
			MCTP_LOGGING_PKT_DROPPED, msg1, msg2);

		if ((status == MCTP_PROTOCOL_INVALID_MSG) || (status == MCTP_PROTOCOL_UNSUPPORTED_MSG)) {
			return mctp_interface_generate_error_packet (interface, tx_message,
				CERBERUS_PROTOCOL_ERROR_INVALID_REQ, status, src_eid, dest_eid, msg_tag,
				response_addr, rx_packet->dest_addr, cmd_set);
		}
		else if (status == MCTP_PROTOCOL_BAD_CHECKSUM) {
			return mctp_interface_generate_error_packet (interface, tx_message,
				CERBERUS_PROTOCOL_ERROR_INVALID_CHECKSUM, crc, src_eid, dest_eid, msg_tag,
				response_addr, rx_packet->dest_addr, cmd_set);
		}
		else {
			mctp_interface_reset_message_processing (interface);
			return status;
		}
	}

	if (dest_eid != interface->eid) {
		return 0;
	}

	if (som) {
		interface->req_buffer.length = 0;
		interface->req_buffer.source_eid = src_eid;
		interface->req_buffer.target_eid = dest_eid;
		interface->start_packet_len = payload_len;
		interface->req_buffer.channel_id = interface->channel_id;
		interface->packet_seq = 0;
		interface->msg_tag = msg_tag;
	}
	else if (interface->start_packet_len == 0) {
		// If this packet is not a SOM, and we haven't received a SOM packet yet
		return mctp_interface_generate_error_packet (interface, tx_message,
			CERBERUS_PROTOCOL_ERROR_OUT_OF_ORDER_MSG, 0, src_eid, dest_eid, msg_tag, response_addr,
			rx_packet->dest_addr, cmd_set);
	}
	else if (packet_seq != interface->packet_seq) {
		return mctp_interface_generate_error_packet (interface, tx_message,
			CERBERUS_PROTOCOL_ERROR_OUT_OF_SEQ_WINDOW, 0, src_eid, dest_eid, msg_tag, response_addr,
			rx_packet->dest_addr, cmd_set);
	}
	else if (msg_tag != interface->msg_tag) {
		return mctp_interface_generate_error_packet (interface, tx_message,
			CERBERUS_PROTOCOL_ERROR_INVALID_REQ, 0, src_eid, dest_eid, msg_tag, response_addr,
			rx_packet->dest_addr, cmd_set);
	}
	else if (src_eid != interface->req_buffer.source_eid) {
		return 0;
	}
	else {
		if (((int) payload_len != interface->start_packet_len) &&
		   !(eom && ((int) payload_len < interface->start_packet_len))) {
			// Can only have different size than SOM if EOM and smaller than SOM
			return mctp_interface_generate_error_packet (interface, tx_message,
				CERBERUS_PROTOCOL_ERROR_INVALID_PACKET_LEN, payload_len, src_eid, dest_eid, msg_tag,
				response_addr, rx_packet->dest_addr, cmd_set);
		}
	}

	if ((payload_len + interface->req_buffer.length) > MCTP_PROTOCOL_MAX_MESSAGE_BODY) {
		return mctp_interface_generate_error_packet (interface, tx_message,
			CERBERUS_PROTOCOL_ERROR_MSG_OVERFLOW, payload_len + interface->req_buffer.length,
			src_eid, dest_eid, msg_tag, response_addr, rx_packet->dest_addr, cmd_set);
	}

	// Assemble packets into message and process message when EOM is received
	memcpy (&interface->req_buffer.data[interface->req_buffer.length], payload, payload_len);
	interface->req_buffer.length += payload_len;
	interface->packet_seq = (interface->packet_seq + 1) % 4;

	if (eom) {
		/* We know the message is one of the two supported types by this point.  If it wasn't, it
		 * would have failed eariler in packet processing. */
		if (MCTP_PROTOCOL_IS_CONTROL_MSG (interface->msg_type)) {
			interface->req_buffer.max_response = MCTP_PROTOCOL_MIN_TRANSMISSION_UNIT;
			status = mctp_interface_control_process_request (interface,	&interface->req_buffer,
				source_addr);
			if (status != 0) {
				debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_MCTP,
					MCTP_LOGGING_CONTROL_FAIL, status, interface->channel_id);
				return status;
			}
		}
		else if (MCTP_PROTOCOL_IS_VENDOR_MSG (interface->msg_type)) {
			header = (struct cerberus_protocol_header*) interface->req_buffer.data;
			cmd_set = header->rq;

			interface->req_buffer.max_response = device_manager_get_max_message_len_by_eid (
				interface->device_manager, src_eid);
			status = interface->cmd_interface->process_request (interface->cmd_interface,
				&interface->req_buffer);

			/* Regardless of the processing status, check to see if the timeout needs adjusting. */
			if (rx_packet->timeout_valid && interface->req_buffer.crypto_timeout) {
				platform_increase_timeout (
					MCTP_PROTOCOL_MAX_CRYPTO_TIMEOUT_MS - MCTP_PROTOCOL_MAX_RESPONSE_TIMEOUT_MS,
					&rx_packet->pkt_timeout);
			}

			if (status == CMD_HANDLER_ERROR_MESSAGE) {
				if (interface->req_buffer.length == sizeof (struct cerberus_protocol_error)) {
					struct cerberus_protocol_error *error_msg =
						(struct cerberus_protocol_error*) interface->req_buffer.data;

					debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_MCTP,
						MCTP_LOGGING_CHANNEL, interface->channel_id, 0);
					debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR,
						DEBUG_LOG_COMPONENT_MCTP, MCTP_LOGGING_ERR_MSG,
						(error_msg->error_code << 24 | src_eid << 16 | dest_eid << 8 | msg_tag),
						error_msg->error_data);
				}

				return 0;
			}

#ifdef ENABLE_DEBUG_COMMANDS
			if (status == ATTESTATION_START_TEST_ESCAPE_SEQ) {
				uint8_t device_num = (uint8_t) (status >> 16);
				status = device_manager_get_device_addr (interface->device_manager, device_num);
				if (!ROT_IS_ERROR (status)) {
					response_addr = status;
					status = interface->cmd_interface->issue_request (interface->cmd_interface,
						CERBERUS_PROTOCOL_GET_DIGEST, NULL, interface->req_buffer.data,
						MCTP_PROTOCOL_MAX_MESSAGE_BODY);
					if (!ROT_IS_ERROR (status)) {
						interface->req_buffer.source_eid =
							device_manager_get_device_eid (interface->device_manager, device_num);
						interface->req_buffer.length = status;
						tag_owner = MCTP_PROTOCOL_TO_REQUEST;
						interface->req_buffer.new_request = true;
						status = 0;
					}
				}

				if (ROT_IS_ERROR (status)) {
					response_addr = source_addr;
				}
			}
#endif

			if (status != 0) {
				return mctp_interface_generate_error_packet (interface, tx_message,
					CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, status, src_eid, dest_eid, msg_tag,
					response_addr, rx_packet->dest_addr, cmd_set);
			}
			else if (interface->req_buffer.length == 0) {
				return mctp_interface_generate_error_packet (interface, tx_message,
					CERBERUS_PROTOCOL_NO_ERROR, status, src_eid, dest_eid, msg_tag, response_addr,
					rx_packet->dest_addr, cmd_set);
			}

			if (interface->req_buffer.length >
				device_manager_get_max_message_len_by_eid (interface->device_manager, src_eid)) {
				return mctp_interface_generate_error_packet (interface, tx_message,
					CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, MCTP_PROTOCOL_MSG_TOO_LARGE, src_eid,
					dest_eid, msg_tag, response_addr, rx_packet->dest_addr, cmd_set);
			}
		}

		if (interface->req_buffer.new_request) {
			tag_owner = MCTP_PROTOCOL_TO_REQUEST;
		}
		else {
			tag_owner = MCTP_PROTOCOL_TO_RESPONSE;
		}

		if (interface->req_buffer.length > 0) {
			interface->packet_seq = 0;
			i_buf = 0;
			som = true;

			max_packet = device_manager_get_max_transmission_unit_by_eid (interface->device_manager,
				src_eid);
			n_packets = MCTP_PROTOCOL_PACKETS_IN_MESSAGE (interface->req_buffer.length, max_packet);

			interface->resp_buffer.msg_size = 0;
			for (i_packet = 0; i_packet < n_packets; ++i_packet) {
				eom = (i_packet == (n_packets - 1));
				payload_len = (interface->req_buffer.length > max_packet) ?
					max_packet : interface->req_buffer.length;

				status = mctp_protocol_construct (&interface->req_buffer.data[i_buf], payload_len,
					&interface->resp_buffer.data[interface->resp_buffer.msg_size],
					sizeof (interface->msg_buffer) - interface->resp_buffer.msg_size,
					rx_packet->dest_addr, interface->req_buffer.source_eid,
					interface->req_buffer.target_eid, som, eom, interface->packet_seq,
					interface->msg_tag, tag_owner, response_addr, &interface->msg_type);
				if (ROT_IS_ERROR (status)) {
					if (MCTP_PROTOCOL_IS_VENDOR_MSG (interface->msg_type)) {
						return mctp_interface_generate_error_packet (interface, tx_message,
							CERBERUS_PROTOCOL_ERROR_UNSPECIFIED, status, src_eid, dest_eid, msg_tag,
							response_addr, rx_packet->dest_addr, cmd_set);
					}
					else {
						return status;
					}
				}

				if (som) {
					interface->resp_buffer.pkt_size = status;
					interface->resp_buffer.dest_addr = response_addr;
				}
				interface->resp_buffer.msg_size += status;

				som = false;
				interface->packet_seq = (interface->packet_seq + 1) % 4;
				interface->req_buffer.length -= payload_len;
				i_buf += payload_len;
			}

			interface->msg_tag = (interface->msg_tag + 1) % 8;

			*tx_message = &interface->resp_buffer;
		}
		else {
			*tx_message = NULL;
		}
	}

	return 0;
}

/**
 * Reset the MCTP layer.  This discards previously received packets and begins looking for a new
 * message.
 *
 * @param interface The MCTP layer to reset.
 */
void mctp_interface_reset_message_processing (struct mctp_interface *interface)
{
	interface->req_buffer.length = 0;
	interface->start_packet_len = 0;
}

/**
 * MCTP interface issue request
 *
 * @param interface MCTP interface instance
 * @param dest_addr Address of device to issue request to
 * @param dest_eid EID of device to issue request to
 * @param src_addr Source address for the request
 * @param src_eid Source EID for the request
 * @param command_id Request command ID
 * @param request_params Paramters for request to issue
 * @param buf Output buffer for packet
 * @param buf_len Maximum buffer length
 * @param msg_type Type of request message
 *
 * @return Output length if completed successfully or an error code.
 */
int mctp_interface_issue_request (struct mctp_interface *interface, uint8_t dest_addr,
	uint8_t dest_eid, uint8_t src_addr, uint8_t src_eid, uint8_t command_id, void *request_params,
	uint8_t *buf, size_t buf_len, uint8_t msg_type)
{
	uint8_t msg_buffer[MCTP_PROTOCOL_MAX_MESSAGE_BODY] = {0};
	int status;

	if ((interface == NULL) || (buf == NULL)) {
		return MCTP_PROTOCOL_INVALID_ARGUMENT;
	}

	if (msg_type == MCTP_PROTOCOL_MSG_TYPE_VENDOR_DEF) {
		status = interface->cmd_interface->issue_request (interface->cmd_interface, command_id,
			request_params, msg_buffer,
			device_manager_get_max_message_len_by_eid (interface->device_manager, dest_eid));
		if (ROT_IS_ERROR (status)) {
			return status;
		}
	}
	else if (msg_type == MCTP_PROTOCOL_MSG_TYPE_CONTROL_MSG) {
		/* Control messages should always fit in a single, required minimum packet. */
		status = mctp_interface_control_issue_request (interface, command_id, request_params,
			msg_buffer, sizeof (msg_buffer));
		if (ROT_IS_ERROR (status)) {
			return status;
		}
	}
	else {
		return MCTP_PROTOCOL_UNSUPPORTED_MSG;
	}

	/* TODO: Handle creating multi-packet messages. */
	status = mctp_protocol_construct (msg_buffer, status, buf, buf_len, src_addr, dest_eid, src_eid,
		true, true, 0, interface->msg_tag, MCTP_PROTOCOL_TO_REQUEST, dest_addr, &msg_type);
	if (ROT_IS_ERROR (status)) {
		return status;
	}

	interface->msg_tag = (interface->msg_tag + 1) % 8;

	return status;
}
