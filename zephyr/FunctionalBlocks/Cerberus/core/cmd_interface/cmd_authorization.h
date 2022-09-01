// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef CMD_CMD_AUTHORIZATION_H_
#define CMD_CMD_AUTHORIZATION_H_

#include <stdint.h>
#include <stddef.h>
#include "status/rot_status.h"
#include "common/authorization.h"


/**
 * Handler for verifying access when attempting to execute commands that require authorization.
 * Each command is an independent authorization context.
 */
struct cmd_authorization {
	/**
	 * Check for authorization to revert the device to bypass mode.
	 *
	 * @param auth Authorization handler to query.
	 * @param token Input or output authorization token, depending on the initial value.  See
	 * {@link struct authorization.authorize}.
	 * @param length Input or output length of the authorization token, depending on the initial
	 * value of the authorization token.  See {@link struct authorization.authorize}.
	 *
	 * @return 0 if the operation is authorized or an error code.  If a token was generated,
	 * CMD_AUTHORIZATION_CHALLENGE will be returned.
	 */
	int (*authorize_revert_bypass) (struct cmd_authorization *auth, uint8_t **token,
		size_t *length);

	/**
	 * Check for authorization to reset the device to factory default configuration.
	 *
	 * @param auth Authorization handler to query.
	 * @param token Input or output authorization token, depending on the initial value.  See
	 * {@link struct authorization.authorize}.
	 * @param length Input or output length of the authorization token, depending on the initial
	 * value of the authorization token.  See {@link struct authorization.authorize}.
	 *
	 * @return 0 if the operation is authorized or an error code.  If a token was generated,
	 * CMD_AUTHORIZATION_CHALLENGE will be returned.
	 */
	int (*authorize_reset_defaults) (struct cmd_authorization *auth, uint8_t **token,
		size_t *length);

	/**
	 * Check for authorization to clear the platform-specific configuratation for the device.
	 *
	 * @param auth Authorization handler to query.
	 * @param token Input or output authorization token, depending on the initial value.  See
	 * {@link struct authorization.authorize}.
	 * @param length Input or output length of the authorization token, depending on the initial
	 * value of the authorization token.  See {@link struct authorization.authorize}.
	 *
	 * @return 0 if the operation is authorized or an error code.  If a token was generated,
	 * CMD_AUTHORIZATION_CHALLENGE will be returned.
	 */
	int (*authorize_clear_platform_config) (struct cmd_authorization *auth, uint8_t **token,
		size_t *length);

	struct authorization *bypass;		/**< Authorization context for reverting to bypass. */
	struct authorization *defaults;		/**< Authorization context for resetting to defaults. */
	struct authorization *platform;		/**< Authorization context for clearing platform config. */
};


int cmd_authorization_init (struct cmd_authorization *auth, struct authorization *bypass,
	struct authorization *defaults, struct authorization *platform);
void cmd_authorization_release (struct cmd_authorization *auth);


#define	CMD_AUTHORIZATION_ERROR(code)		ROT_ERROR (ROT_MODULE_CMD_AUTHORIZATION, code)

/**
 * Error codes that can be generated by an observer manager.
 */
enum {
	CMD_AUTHORIZATION_INVALID_ARGUMENT = CMD_AUTHORIZATION_ERROR (0x00),	/**< Input parameter is null or not valid. */
	CMD_AUTHORIZATION_NO_MEMORY = CMD_AUTHORIZATION_ERROR (0x01),			/**< Memory allocation failed. */
	CMD_AUTHORIZATION_BYPASS_FAILED = CMD_AUTHORIZATION_ERROR (0x02),		/**< Failed authorization to revert to bypass mode. */
	CMD_AUTHORIZATION_DEFAULTS_FAILED = CMD_AUTHORIZATION_ERROR (0x03),		/**< Failed authorization to restore defaults. */
};


#endif /* CMD_CMD_AUTHORIZATION_H_ */