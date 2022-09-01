// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "spi_flash.h"
#include "flash/flash_common.h"
#include "flash/flash_logging.h"


/* Status bits indicating when flash is operating in 4-byte address mode. */
#define	MACRONIX_4BYTE_STATUS		(1U << 5)
#define	WINBOND_4BYTE_STATUS		(1U << 0)
#define	MICRON_4BYTE_STATES			(1U << 0)

/* Config bits indicating address mode on reset. */
#define	WINBOND_4BYTE_DEFAULT		(1U << 1)
#define	MICRON_4BYTE_DEFAULT		(1U << 0)

/* Status bits indicating when flash has QSPI enabled. */
#define	RESET_HOLD_ENABLE			(1U << 4)
#define	QSPI_ENABLE_BIT1			(1U << 1)
#define	QSPI_ENABLE_BIT6			(1U << 6)
#define	QSPI_ENABLE_BIT7			(1U << 7)


/**
 * Check the requested operation to ensure it is valid for the device.
 */
#define	SPI_FLASH_BOUNDS_CHECK(bytes, addr, len) \
	if (addr >= bytes) { \
		return SPI_FLASH_ADDRESS_OUT_OF_RANGE; \
	} \
	\
	if ((addr + len) > bytes) { \
		return SPI_FLASH_OPERATION_OUT_OF_RANGE; \
	}


/**
 * Configure the read command for the flash device.
 *
 * @param flash The flash interface to configure.
 * @param command Read command information to use for configuration.
 * @param opcode_4byte The read command to use in 4-byte mode.
 * @param use_4byte Flag indicating if 4-byte mode is enabled.
 * @param flags Transaction flags for the read.
 */
static void spi_flash_set_read_command (struct spi_flash *flash,
	struct spi_flash_sfdp_read_cmd *command, uint8_t opcode_4byte, bool use_4byte, uint16_t flags)
{
	flash->command.read_dummy = command->dummy_bytes;
	flash->command.read_mode = command->mode_bytes;
	flash->command.read_flags = flags;
	if (use_4byte) {
		flash->command.read = opcode_4byte;
		flash->command.read_flags |= FLASH_FLAG_4BYTE_ADDRESS;
	}
	else {
		flash->command.read = command->opcode;
	}
}

/**
 * Configure the program and erase commands for the flash device.
 *
 * @param flash The flash interface to configure.
 */
static void spi_flash_set_write_commands (struct spi_flash *flash)
{
	if ((flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) ==
		(FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) {
		flash->command.write = FLASH_CMD_4BYTE_PP;
		flash->command.write_flags = FLASH_FLAG_4BYTE_ADDRESS;

		flash->command.erase_sector = FLASH_CMD_4BYTE_4K_ERASE;
		flash->command.sector_flags = FLASH_FLAG_4BYTE_ADDRESS;

		flash->command.erase_block = FLASH_CMD_4BYTE_64K_ERASE;
		flash->command.block_flags = FLASH_FLAG_4BYTE_ADDRESS;
	}
}

/**
 * Configure the command set for the device based on its capabilities.
 *
 * @param flash The flash interface to configure.
 * @param read Information from SFDP for read commands.
 * @param sfdp SFDP tables for additional command information.
 */
static void spi_flash_set_device_commands (struct spi_flash *flash,
	struct spi_flash_sfdp_read_commands *read, struct spi_flash_sfdp_basic_table *sfdp)
{
	bool use_4byte;

	use_4byte = ((flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) ==
		(FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR));

	if (flash->capabilities & FLASH_CAP_QUAD_1_4_4) {
		spi_flash_set_read_command (flash, &read->quad_1_4_4, FLASH_CMD_4BYTE_QIO_READ, use_4byte,
			FLASH_FLAG_QUAD_ADDR | FLASH_FLAG_QUAD_DATA);
	}
	else if (flash->capabilities & FLASH_CAP_QUAD_1_1_4) {
		spi_flash_set_read_command (flash, &read->quad_1_1_4, FLASH_CMD_4BYTE_QUAD_READ, use_4byte,
			FLASH_FLAG_QUAD_DATA);
	}
	else if (flash->capabilities & FLASH_CAP_DUAL_1_2_2) {
		spi_flash_set_read_command (flash, &read->dual_1_2_2, FLASH_CMD_4BYTE_DIO_READ, use_4byte,
			FLASH_FLAG_DUAL_ADDR | FLASH_FLAG_DUAL_DATA);
	}
	else if (flash->capabilities & FLASH_CAP_DUAL_1_1_2) {
		spi_flash_set_read_command (flash, &read->dual_1_1_2, FLASH_CMD_4BYTE_DUAL_READ, use_4byte,
			FLASH_FLAG_DUAL_DATA);
	}
	else if (use_4byte) {
		if (flash->use_fast_read) {
			flash->command.read = FLASH_CMD_4BYTE_FAST_READ;
			flash->command.read_dummy = 1;
		}
		else {
			flash->command.read = FLASH_CMD_4BYTE_READ;
		}
		flash->command.read_flags = FLASH_FLAG_4BYTE_ADDRESS;
	}

	spi_flash_set_write_commands (flash);

	if (sfdp) {
		spi_flash_sfdp_get_reset_command (sfdp, &flash->command.reset);
		spi_flash_sfdp_get_deep_powerdown_commands (sfdp, &flash->command.enter_pwrdown,
			&flash->command.release_pwrdown);
	}
}

/**
 * Completely initialize a SPI flash interface and device so it is read for use.  This includes:
 * 		- Initializing the SPI flash interface.
 * 		- Configuring the interface and device based on discovered properties.
 * 		- Detecting the address mode of the device.
 *
 * @param flash The flash interface to initialize.
 * @param spi The SPI master connected to the flash.
 * @param fast_read Flag indicating if the FAST_READ command should be used for SPI reads.
 * @param wake_device Flag indicating if the device should be removed from deep power down.
 * @param reset_device Flag indicating if the device should be reset prior to initialization.
 * @param drive_strength Flag indicating if the device output drive strength should be configured.
 *
 * @return 0 if the SPI flash was successfully initialized or an error code.
 */
int spi_flash_initialize_device (struct spi_flash *flash, struct flash_master *spi, bool fast_read,
	bool wake_device, bool reset_device, bool drive_strength)
{
	struct spi_flash_sfdp sfdp;
	int status;

	if (fast_read) {
		status = spi_flash_init_fast_read (flash, spi);
	}
	else {
		status = spi_flash_init (flash, spi);
	}
	if (status != 0) {
		return status;
	}

	if (wake_device) {
		status = spi_flash_deep_power_down (flash, 0);
		if (status != 0) {
			goto fail_flash;
		}
	}

	status = spi_flash_get_device_id (flash, NULL, NULL);
	if (status != 0) {
		goto fail_flash;
	}

	if ((flash->device_id[0] == 0xff) || (flash->device_id[0] == 0x00)) {
		status = SPI_FLASH_NO_DEVICE;
		goto fail_flash;
	}

	status = spi_flash_sfdp_init (&sfdp, spi);
	if (status != 0) {
		goto fail_flash;
	}

	status = spi_flash_discover_device_properties (flash, &sfdp);
	if (status != 0) {
		goto fail_sfdp;
	}

	/* Make sure the device is not writing any data before we proceed.  Resets will corrupt the
	 * flash and register writes will fail if a write is currently in progress. */
	status = spi_flash_wait_for_write (flash, 30000);
	if (status != 0) {
		goto fail_sfdp;
	}

	if (reset_device) {
		status = spi_flash_reset_device (flash);
		if (status != 0) {
			goto fail_sfdp;
		}
	}

	if (drive_strength) {
		status = spi_flash_configure_drive_strength (flash);
		if (status != 0) {
			goto fail_sfdp;
		}
	}

	if ((flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) ==
		(FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) {
		status = spi_flash_detect_4byte_address_mode (flash);
		if (status != 0) {
			goto fail_sfdp;
		}
	}

	if (flash->command.read_flags & FLASH_FLAG_QUAD_DATA) {
		status = spi_flash_enable_quad_spi (flash, 1);
		if (status != 0) {
			goto fail_sfdp;
		}
	}

	status = spi_flash_clear_block_protect (flash);
	if (status != 0) {
		goto fail_sfdp;
	}

	spi_flash_sfdp_release (&sfdp);
	return 0;

fail_sfdp:
	spi_flash_sfdp_release (&sfdp);
fail_flash:
	spi_flash_release (flash);
	return status;
}

/**
 * Initialize a SPI flash device from a saved context.  Upon completion, the interface will be ready
 * to use, but no transaction with the flash device will be performed.  This could leave the
 * interface and device in an inconsistent state (e.g. the current address mode).  It is recommended
 * that the interface be synchronized with the flash when SPI accesses are possible.
 *
 * @param flash The flash interface to initialize.
 * @param spi The SPI master connected to the flash.
 * @param info The saved device information to use for interface initialization.
 *
 * @return 0 if the flash interface was successfully initialized or an error code.
 */
int spi_flash_restore_device (struct spi_flash *flash, struct flash_master *spi,
	struct spi_flash_device_info *info)
{
	int status;

	if (info == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	if (info->use_fast_read) {
		status = spi_flash_init_fast_read (flash, spi);
	}
	else {
		status = spi_flash_init (flash, spi);
	}
	if (status != 0) {
		return status;
	}

	memcpy (flash->device_id, info->device_id, sizeof (flash->device_id));
	flash->device_size = info->device_size;
	flash->capabilities = info->capabilities;
	flash->use_busy_flag = !!(info->flags & SPI_FLASH_DEVICE_INFO_BUSY_FLAG);
	flash->switch_4byte = (enum spi_flash_sfdp_4byte_addressing) info->switch_4byte;
	flash->reset_3byte = !!(info->flags & SPI_FLASH_DEVICE_INFO_RESET_3BYTE);
	flash->quad_enable = (enum spi_flash_sfdp_quad_enable) info->quad_enable;
	flash->sr1_volatile = !!(info->flags & SPI_FLASH_DEVICE_INFO_SR1_VOLATILE);

	flash->command.read = info->read_opcode;
	flash->command.read_dummy = info->read_dummy;
	flash->command.read_mode = info->read_mode;
	flash->command.read_flags = info->read_flags;

	spi_flash_set_write_commands (flash);
	flash->command.reset = info->reset_opcode;
	flash->command.enter_pwrdown = info->enter_pwrdown;
	flash->command.release_pwrdown = info->release_pwrdown;

	return 0;
}

/**
 * Initialize the SPI flash interface.
 *
 * This is not sufficient to be able to fully access the SPI flash device.  Use
 * {@link spi_flash_initialize_device} for complete device initialization.
 *
 * @param flash The flash interface to initialize.
 * @param spi The SPI master connected to the flash.
 *
 * @return 0 if the flash interface was initialized or an error code.
 */
int spi_flash_init (struct spi_flash *flash, struct flash_master *spi)
{
	int status;

	if ((flash == NULL) || (spi == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	memset (flash, 0, sizeof (struct spi_flash));

	status = platform_mutex_init (&flash->lock);
	if (status != 0) {
		return status;
	}

	flash->command.read = FLASH_CMD_READ;
	flash->command.write = FLASH_CMD_PP;
	flash->command.erase_sector = FLASH_CMD_4K_ERASE;
	flash->command.erase_block = FLASH_CMD_64K_ERASE;
	flash->command.enter_pwrdown = FLASH_CMD_DP;
	flash->command.release_pwrdown = FLASH_CMD_RDP;

	flash->spi = spi;
	flash->device_id[0] = 0xff;
	flash->capabilities = (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR);

	flash->base.get_device_size = (int (*) (struct flash*, uint32_t*)) spi_flash_get_device_size;
	flash->base.read = (int (*) (struct flash*, uint32_t, uint8_t*, size_t)) spi_flash_read;
	flash->base.get_page_size = (int (*) (struct flash*, uint32_t*)) spi_flash_get_page_size;
	flash->base.minimum_write_per_page =
		(int (*) (struct flash*, uint32_t*)) spi_flash_minimum_write_per_page;
	flash->base.write = (int (*) (struct flash*, uint32_t, const uint8_t*, size_t)) spi_flash_write;
	flash->base.get_sector_size = (int (*) (struct flash*, uint32_t*)) spi_flash_get_sector_size;
	flash->base.sector_erase = (int (*) (struct flash*, uint32_t)) spi_flash_sector_erase;
	flash->base.get_block_size = (int (*) (struct flash*, uint32_t*)) spi_flash_get_block_size;
	flash->base.block_erase = (int (*) (struct flash*, uint32_t)) spi_flash_block_erase;
	flash->base.chip_erase = (int (*) (struct flash*)) spi_flash_chip_erase;

	return 0;
}

/**
 * Initialize the SPI flash interface.  The FAST_READ command will be used for SPI reads.
 *
 * This is not sufficient to be able to fully access the SPI flash device.  Use
 * {@link spi_flash_initialize_device} for complete device initialization.
 *
 * @param flash The flash interface to initialize.
 * @param spi The SPI master connected to the flash.
 *
 * @return 0 if the flash interface was initialized or an error code.
 */
int spi_flash_init_fast_read (struct spi_flash *flash, struct flash_master *spi)
{
	int status = spi_flash_init (flash, spi);
	if (status != 0) {
		return status;
	}

	flash->use_fast_read = true;
	flash->command.read = FLASH_CMD_FAST_READ;
	flash->command.read_dummy = 1;

	return 0;
}

/**
 * Release the SPI flash interface.
 *
 * @param flash The flash interface to release.
 */
void spi_flash_release (struct spi_flash *flash)
{
	if (flash) {
		platform_mutex_free (&flash->lock);
		memset (flash, 0, sizeof (struct spi_flash));
	}
}

/**
 * Save the SPI device context.  This will allow a new SPI flash interface to be created to
 * communicate with the flash device without needing to query the device.
 *
 * @param flash The flash interface to save.
 * @param info The context that will be updated for the flash device.
 *
 * @return 0 if the flash device context was successfully saved or an error code.
 */
int spi_flash_save_device_info (struct spi_flash *flash, struct spi_flash_device_info *info)
{
	if ((flash == NULL) || (info == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	info->version = SPI_FLASH_DEVICE_INFO_VERSION;
	memcpy (info->device_id, flash->device_id, sizeof (info->device_id));
	info->device_size = flash->device_size;
	info->capabilities = flash->capabilities;
	info->use_fast_read = flash->use_fast_read;
	info->read_opcode = flash->command.read;
	info->read_dummy = flash->command.read_dummy;
	info->read_mode = flash->command.read_mode;
	info->read_flags = flash->command.read_flags;
	info->reset_opcode = flash->command.reset;
	info->enter_pwrdown = flash->command.enter_pwrdown;
	info->release_pwrdown = flash->command.release_pwrdown;
	info->switch_4byte = flash->switch_4byte;
	info->quad_enable = flash->quad_enable;

	info->flags = 0;
	if (flash->use_busy_flag) {
		info->flags |= SPI_FLASH_DEVICE_INFO_BUSY_FLAG;
	}
	if (flash->reset_3byte) {
		info->flags |= SPI_FLASH_DEVICE_INFO_RESET_3BYTE;
	}
	if (flash->sr1_volatile) {
		info->flags |= SPI_FLASH_DEVICE_INFO_SR1_VOLATILE;
	}

	return 0;
}

/**
 * Send a write command to the flash that only sends the command code.
 *
 * @param flash The flash instance to use to send the command.
 * @param cmd The command code to send to the device.
 *
 * @return 0 if the command was successfully sent or an error code.
 */
static int spi_flash_simple_command (struct spi_flash *flash, uint8_t cmd)
{
	struct flash_xfer xfer;

	FLASH_XFER_INIT_CMD_ONLY (xfer, cmd, 0);
	return flash->spi->xfer (flash->spi, &xfer);
}

/**
 * Send the write enable command to the flash device.
 *
 * @param flash The flash instance to use to send the command.
 *
 * @return 0 if the command was successfully sent or an error code.
 */
static int spi_flash_write_enable (struct spi_flash *flash)
{
	return spi_flash_simple_command (flash, FLASH_CMD_WREN);
}

/**
 * Send the volatile write enable command to the flash device.
 *
 * @param flash The flash instance to use to send the command.
 *
 * @return 0 if the command was successfully sent or an error code.
 */
static int spi_flash_volatile_write_enable (struct spi_flash *flash)
{
	return spi_flash_simple_command (flash, FLASH_CMD_VOLATILE_WREN);
}

/**
 * Determine if the flash is currently executing a write command.
 *
 * @param flash The flash instance to check.
 *
 * @return 0 if no write is in progress, 1 if there is, or an error code.
 */
static int spi_flash_is_wip_set (struct spi_flash *flash)
{
	struct flash_xfer xfer;
	uint8_t reg;
	int status;

	if (!flash->use_busy_flag) {
		FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR, &reg, 1, 0);
	}
	else {
		FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR_FLAG, &reg, 1, 0);
	}

	status = flash->spi->xfer (flash->spi, &xfer);
	if (status == 0) {
		if (!flash->use_busy_flag) {
			return ((reg & FLASH_STATUS_WIP) != 0);
		}
		else {
			return ((reg & FLASH_FLAG_STATUS_READY) == 0);
		}
	}
	else {
		return status;
	}
}

/**
 * Wait for a write operation to complete.
 *
 * @param flash The flash instance that is executing a write operation.
 * @param timeout The maximum number of milliseconds to wait for completion.  A negative number will
 * wait forever.  0 will return immediately.
 * @param no_sleep Flag indicating no sleep should be inserted between status reads.
 *
 * @return 0 if the write was completed or an error code.
 */
static int spi_flash_wait_for_write_completion (struct spi_flash *flash, int32_t timeout,
	uint8_t no_sleep)
{
	platform_clock timeout_val;
	int done = 0;
	int status;

	if (timeout > 0) {
		status = platform_init_timeout (timeout, &timeout_val);
		if (status) {
			return status;
		}
	}

	do {
		status = spi_flash_is_wip_set (flash);
		if (status == 0) {
			done = 1;
		}
		else if (status == 1) {
			status = 0;

			if ((timeout > 0) && (platform_has_timeout_expired (&timeout_val) == 1)) {
				status = SPI_FLASH_WIP_TIMEOUT;
			}
			else if (timeout == 0) {
				status = SPI_FLASH_WIP_TIMEOUT;
			}

			if (status == 0) {
				if (!no_sleep) {
					platform_msleep (10);
				}
			}
		}
	} while ((status == 0) && !done);

	return status;
}

/**
 * Send a write command that writes to register that requires no addressing.  This will block until
 * the register write has completed.
 *
 * @param flash The flash instance to use to send the command.
 * @param cmd The command code that writes the register.
 * @param data The data to write to the register.
 * @param length The length of the data to write.
 * @param volatile_wren Flag indicating the volatile write enable command should be used.
 *
 * @return 0 if the command was successfully completed or an error code.
 */
static int spi_flash_write_register (struct spi_flash *flash, uint8_t cmd, uint8_t *data,
	size_t length, bool volatile_wren)
{
	struct flash_xfer xfer;
	int status;

	status = spi_flash_is_wip_set (flash);
	if (status != 0) {
		return (status == 1) ? SPI_FLASH_WRITE_IN_PROGRESS : status;
	}

	if (volatile_wren) {
		status = spi_flash_volatile_write_enable (flash);
	}
	else {
		status = spi_flash_write_enable (flash);
	}
	if (status != 0) {
		return status;
	}

	FLASH_XFER_INIT_WRITE_REG (xfer, cmd, data, length, 0);
	status = flash->spi->xfer (flash->spi, &xfer);
	if (status != 0) {
		return status;
	}

	return spi_flash_wait_for_write_completion (flash, -1, 1);
}

/**
 * Discover device properties necessary for operation through SFDP.  This must be done prior to
 * using the interface to the device.
 *
 * This call supersedes {@link spi_flash_set_device_size}, which should no longer be used outside
 * of unit testing.  This call sets the device size and will also detect and configure many other
 * relevant parameters.
 *
 * @param flash The flash interface to configure.
 * @param sfdp The SFDP interface to use for property detection.  The SFDP instance is not
 * maintained internally and it can be managed independently of the SPI flash interface.
 *
 * @return 0 if the SPI flash properties were successfully detected or an error code.
 */
int spi_flash_discover_device_properties (struct spi_flash *flash, struct spi_flash_sfdp *sfdp)
{
	struct spi_flash_sfdp_basic_table parameters;
	uint32_t spi_capabilities;
	struct spi_flash_sfdp_read_commands read;
	int status;

	if ((flash == NULL) || (sfdp == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	status = spi_flash_sfdp_basic_table_init (&parameters, sfdp);
	if (status != 0) {
		return status;
	}

	platform_mutex_lock (&flash->lock);

	spi_flash_sfdp_get_device_capabilities (&parameters, &flash->capabilities);
	spi_flash_sfdp_get_read_commands (&parameters, &read);

	spi_capabilities = flash->spi->capabilities (flash->spi) & flash->capabilities;
	if ((spi_capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) !=
		(flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR))) {
		status = SPI_FLASH_INCOMPATIBLE_SPI_MASTER;
		goto exit;
	}

	flash->capabilities = spi_capabilities;

	switch (flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) {
		case (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR):
			if (!spi_flash_sfdp_supports_4byte_commands (&parameters)) {
				/* We expect the flash device to support explicit 4-byte address commands.  If it
				 * does not, we can't support communicating with that flash.  It is possible to add
				 * support for these devices using enter/exit 4-byte commands, EAR, etc., but that
				 * is a lot more complicated.  Since devices that don't support these commands are
				 * a minority of devices or older, there is not much benefit to adding this now. */
				status = SPI_FLASH_NO_4BYTE_CMDS;
				goto exit;
			}

			flash->reset_3byte = spi_flash_sfdp_exit_4byte_mode_on_reset (&parameters);
			break;

		case FLASH_CAP_4BYTE_ADDR:
			flash->addr_mode = FLASH_FLAG_4BYTE_ADDRESS;
			break;
	}

	spi_flash_set_device_commands (flash, &read, &parameters);

	status = spi_flash_sfdp_get_4byte_mode_switch (&parameters, &flash->switch_4byte);
	if (status != 0) {
		goto exit;
	}

	status = spi_flash_sfdp_get_quad_enable (&parameters, &flash->quad_enable);
	if (status != 0) {
		goto exit;
	}

	status = spi_flash_sfdp_get_device_size (&parameters);
	if (ROT_IS_ERROR (status)) {
		goto exit;
	}

	flash->device_size = status;
	flash->use_busy_flag = spi_flash_sfdp_use_busy_flag_status (&parameters);
	flash->sr1_volatile = spi_flash_sfdp_use_volatile_write_enable (&parameters);

	status = 0;

exit:
	platform_mutex_unlock (&flash->lock);
	spi_flash_sfdp_basic_table_release (&parameters);
	return status;
}

/**
 * Set the capacity of the flash device.  This must be set before using the interface to the
 * device.
 *
 * NOTE: This call should NOT be used in normal application code.
 * {@link spi_flash_discover_device_properties} should be used to set the device size instead.
 *
 * @param flash The flash instance to configure.
 * @param bytes The capacity of the physical flash device, in bytes.
 *
 * @return 0 if the interface was configured successfully or an error code.
 */
int spi_flash_set_device_size (struct spi_flash *flash, uint32_t bytes)
{
	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	flash->device_size = bytes;
	if (bytes > 0x1000000) {
		spi_flash_set_device_commands (flash, NULL, NULL);
	}

	platform_mutex_unlock (&flash->lock);

	return 0;
}

/**
 * Read the device ID from the SPI flash.
 *
 * @param flash The flash to identify.
 * @param vendor The buffer that will hold the vender ID.  Null to ignore vendor ID.
 * @param device The buffer that will hold the device ID.  Null to ignore device ID.
 *
 * @return 0 if the identifier was successfully read or an error code.
 */
int spi_flash_get_device_id (struct spi_flash *flash, uint8_t *vendor, uint16_t *device)
{
	struct flash_xfer xfer;
	int status = 0;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	if ((flash->device_id[0] == 0xff) || (flash->device_id[0] == 0)) {
		FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDID, flash->device_id, sizeof (flash->device_id),
			0);

		status = flash->spi->xfer (flash->spi, &xfer);
		if (status != 0) {
			flash->device_id[0] = 0xff;
			goto exit;
		}
	}

	if (vendor != NULL) {
		*vendor = flash->device_id[0];
	}
	if (device != NULL) {
		*device = (flash->device_id[1] << 8) | flash->device_id[2];
	}

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Get the size of the flash device.
 *
 * @param flash The flash to query.
 * @param bytes The buffer that will hold the number of bytes in the device.
 *
 * @return 0 if the device size was successfully read or an error code.
 */
int spi_flash_get_device_size (struct spi_flash *flash, uint32_t *bytes)
{
	if ((flash == NULL) || (bytes == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	*bytes = flash->device_size;
	return 0;
}

/**
 * Soft reset the SPI flash device.
 *
 * @param flash The flash to reset.
 *
 * @return 0 if the device was successfully reset or an error code.
 */
int spi_flash_reset_device (struct spi_flash *flash)
{
	int status;
	uint16_t rst_addr_mode;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	if (!flash->command.reset) {
		return SPI_FLASH_RESET_NOT_SUPPORTED;
	}

	if (flash->reset_3byte) {
		/* If 4-byte address mode is cleared on reset, check device settings to see if this
		 * property has been overriden. */
		status = spi_flash_is_4byte_address_mode_on_reset (flash);
		if ((status == 0) || (status == SPI_FLASH_UNSUPPORTED_DEVICE)) {
			rst_addr_mode = 0;
		}
		else if (status == 1) {
			rst_addr_mode = FLASH_FLAG_4BYTE_ADDRESS;
		}
		else {
			return status;
		}
	}
	else {
		rst_addr_mode = flash->addr_mode;
	}

	platform_mutex_lock (&flash->lock);

	status = spi_flash_is_wip_set (flash);
	if (status != 0) {
		status = (status == 1) ? SPI_FLASH_WRITE_IN_PROGRESS : status;
		goto exit;
	}

	if (flash->command.reset == FLASH_CMD_RST) {
		status = spi_flash_simple_command (flash, FLASH_CMD_RSTEN);
		if (status != 0) {
			goto exit;
		}
	}

	status = spi_flash_simple_command (flash, flash->command.reset);
	if (status == 0) {
		flash->addr_mode = rst_addr_mode;

		platform_msleep (100);
	}

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Clear the block protect bits in the main status register.
 *
 * @param flash The flash device to configure.
 *
 * @return 0 if the command was successful or an error code.
 */
int spi_flash_clear_block_protect (struct spi_flash *flash)
{
	struct flash_xfer xfer;
	uint8_t reg[2];
	uint8_t cmd_len = 1;
	uint8_t mask = 0x83;
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	/* Depending on the quad enable bit, the block clear needs to be handled differently:
	 *   - If the quad bit is in SR1, then we need to be sure not to clear it.
	 *   - On some devices, writing only 1 byte to SR1 will automatically clear SR2.  On these
	 *     devices we need to write both SR1 and SR2 to ensure the quad bit doesn't get cleared. */
	switch (flash->quad_enable) {
		case SPI_FLASH_SFDP_QUAD_QE_BIT6_SR1:
			mask = 0xc3;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2:
			cmd_len = 2;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35:
			FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR2, &reg[1], 1, 0);
			status = flash->spi->xfer (flash->spi, &xfer);
			if (status != 0) {
				goto exit;
			}
			break;

		default:
			break;
	}

	FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR, reg, cmd_len, 0);
	status = flash->spi->xfer (flash->spi, &xfer);
	if (status != 0) {
		goto exit;
	}

	if (reg[0] & ~mask) {
		if (flash->quad_enable == SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35) {
			cmd_len = 2;
		}

		reg[0] &= mask;
		status = spi_flash_write_register (flash, FLASH_CMD_WRSR, reg, cmd_len,
			flash->sr1_volatile);
	}

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Transition the flash device to/from deep power down mode.  While in deep power down mode, no
 * commands will be executed by the flash device, except the command to release it from deep power
 * down.
 *
 * @param flash The flash device to configure.
 * @param enable 1 to enter deep power down mode or 0 to release the device from this mode.
 *
 * @return 0 if the command was successfully sent to the device or an error code.
 */
int spi_flash_deep_power_down (struct spi_flash *flash, uint8_t enable)
{
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	if (!flash->command.enter_pwrdown) {
		return (enable) ? SPI_FLASH_PWRDOWN_NOT_SUPPORTED : 0;
	}

	platform_mutex_lock (&flash->lock);

	if (enable) {
		status = spi_flash_simple_command (flash, flash->command.enter_pwrdown);
	}
	else {
		status = spi_flash_simple_command (flash, flash->command.release_pwrdown);
	}

	if (status == 0) {
		platform_msleep (100);
	}

	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Determine if the address mode of the flash device can be changed.
 *
 * @param flash The flash to query.
 *
 * @return 1 if the address mode of the device is fixed, 0 if it can be changed, or an error code.
 */
int spi_flash_is_address_mode_fixed (struct spi_flash *flash)
{
	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	return ((flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) !=
		(FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR));
}

/**
 * Determine if the flash device requires Write Enable to be set in order to switch address modes.
 *
 * @param flash The flash to query.
 *
 * @return 1 if write enable is required, 0 if it is not, or an error code.  If the address mode
 * cannot be switched, SPI_FLASH_ADDR_MODE_FIXED will be returned.
 */
int spi_flash_address_mode_requires_write_enable (struct spi_flash *flash)
{
	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	if (spi_flash_is_address_mode_fixed (flash)) {
		return SPI_FLASH_ADDR_MODE_FIXED;
	}

	return (flash->switch_4byte == SPI_FLASH_SFDP_4BYTE_MODE_COMMAND_WRITE_ENABLE);
}

/**
 * Determine if the flash device defaults to 4-byte address mode on device resets.
 *
 * @param flash The flash to query.
 *
 * @return 1 if the device defaults to 4-byte mode, 0 if 3-byte mode is the default, or an error
 * code.
 */
int spi_flash_is_4byte_address_mode_on_reset (struct spi_flash *flash)
{
	struct flash_xfer xfer;
	uint8_t vendor;
	uint8_t cmd;
	uint8_t mask;
	uint8_t reg;
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	/* Handle fixed address mode. */
	switch (flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) {
		case FLASH_CAP_3BYTE_ADDR:
			return 0;

		case FLASH_CAP_4BYTE_ADDR:
			return 1;
	}

	/* Detecting address state on reset is vendor dependent. */
	status = spi_flash_get_device_id (flash, &vendor, NULL);
	if (status != 0) {
		return status;
	}

	switch (vendor) {
		case FLASH_ID_MACRONIX:
			cmd = 0;
			mask = 0;
			break;

		case FLASH_ID_WINBOND:
			cmd = FLASH_CMD_RDSR3;
			mask = WINBOND_4BYTE_DEFAULT;
			break;

		case FLASH_ID_MICRON:
			cmd = FLASH_CMD_RD_NV_CFG;
			mask = MICRON_4BYTE_DEFAULT;
			break;

		default:
			return SPI_FLASH_UNSUPPORTED_DEVICE;
	}

	if (cmd) {
		platform_mutex_lock (&flash->lock);

		FLASH_XFER_INIT_READ_REG (xfer, cmd, &reg, 1, 0);
		status = flash->spi->xfer (flash->spi, &xfer);

		platform_mutex_unlock (&flash->lock);
		if (status != 0) {
			return status;
		}
	}

	return (vendor == FLASH_ID_MICRON) ? !(reg & mask) : !!(reg & mask);
}

/**
 * Determine if the requested address mode is supported by the flash device.
 *
 * @param flash The flash to query.
 * @param mode 1 for 4-byte mode or 0 3-byte mode.
 *
 * @return 0 if the address mode is supported or an error code.
 */
static int spi_flash_supports_address_mode (struct spi_flash *flash, uint8_t mode)
{
	switch (flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) {
		case FLASH_CAP_3BYTE_ADDR:
			return (mode) ? SPI_FLASH_UNSUPPORTED_ADDR_MODE : SPI_FLASH_ADDR_MODE_FIXED;

		case FLASH_CAP_4BYTE_ADDR:
			return (mode) ? SPI_FLASH_ADDR_MODE_FIXED : SPI_FLASH_UNSUPPORTED_ADDR_MODE;
	}

	return 0;
}

/**
 * Enable or disable 4-byte address mode for commands sent to the flash device.
 *
 * @param flash The flash to configure the address mode for.
 * @param enable 1 to enable 4-byte mode or 0 to disable it (and go to 3-byte mode).
 *
 * @return 0 if the address mode was successfully configured or an error code.
 */
int spi_flash_enable_4byte_address_mode (struct spi_flash *flash, uint8_t enable)
{
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	status = spi_flash_supports_address_mode (flash, enable);
	if (status != 0) {
		if (status == SPI_FLASH_ADDR_MODE_FIXED) {
			status = 0;
		}
		goto exit;
	}

	if (flash->switch_4byte == SPI_FLASH_SFDP_4BYTE_MODE_COMMAND_WRITE_ENABLE) {
		status = spi_flash_write_enable (flash);
		if (status != 0) {
			goto exit;
		}
	}

	if (enable) {
		status = spi_flash_simple_command (flash, FLASH_CMD_EN4B);
		if (status == 0) {
			flash->addr_mode = FLASH_FLAG_4BYTE_ADDRESS;
		}
	}
	else {
		status = spi_flash_simple_command (flash, FLASH_CMD_EX4B);
		if (status == 0) {
			flash->addr_mode = 0;
		}
	}

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Indicate if the SPI flash is operating in 4-byte address mode.
 *
 * This is just the state of the interface communicating with the SPI flash, so no commands are sent
 * to the device.  The interface and device must always be in sync regarding this setting.  Use
 * {@link spi_flash_detect_4byte_address_mode} or {@link spi_flash_enable_4byte_address_mode} to
 * detect or configure the device state.
 *
 * @param flash The flash device to query.
 *
 * @return 1 if 4-byte address mode is enabled, 0 if it is not, or an error code.
 */
int spi_flash_is_4byte_address_mode (struct spi_flash *flash)
{
	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	return !!(flash->addr_mode);
}

/**
 * Read the SPI flash state to determine what address mode the device is operating in.
 *
 * @param flash The flash to device query.
 *
 * @return 0 if the address mode was successfully determined or an error code.
 */
int spi_flash_detect_4byte_address_mode (struct spi_flash *flash)
{
	struct flash_xfer xfer;
	uint8_t vendor;
	uint8_t cmd = FLASH_CMD_RDSR3;
	uint8_t reg;
	int status;
	int mask;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	if ((flash->capabilities & (FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) !=
		(FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR)) {
		return 0;
	}

	status = spi_flash_get_device_id (flash, &vendor, NULL);
	if (status != 0) {
		return status;
	}

	switch (vendor) {
		case FLASH_ID_MACRONIX:
			mask = MACRONIX_4BYTE_STATUS;
			break;

		case FLASH_ID_WINBOND:
			mask = WINBOND_4BYTE_STATUS;
			break;

		case FLASH_ID_MICRON:
			mask = MICRON_4BYTE_STATES;
			cmd = FLASH_CMD_RDSR_FLAG;
			break;

		default:
			return SPI_FLASH_UNSUPPORTED_DEVICE;
	}

	platform_mutex_lock (&flash->lock);

	FLASH_XFER_INIT_READ_REG (xfer, cmd, &reg, 1, 0);
	status = flash->spi->xfer (flash->spi, &xfer);
	if (status != 0) {
		goto exit;
	}

	flash->addr_mode = (reg & mask) ? FLASH_FLAG_4BYTE_ADDRESS : 0;

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Specify the addressing mode that should be used to communicate with the SPI flash without
 * sending any SPI commands.
 *
 * NOTE: Since no commands are sent to flash, it is possible to configure the interface incorrectly
 * and cause flash access errors and/or system hangs.  Be sure this is ONLY used in situations where
 * the state of the flash device is 100% known.  Prefer {@link spi_flash_detect_4byte_address_mode}
 * in situations where it is not desired to change the current state of the flash device.
 *
 * @param flash The flash instance to update.
 * @param enable 1 to use 4-byte more or 0 for 3-byte mode.
 *
 * @return 0 if the addressing mode was successfully set or an error code.
 */
int spi_flash_force_4byte_address_mode (struct spi_flash *flash, uint8_t enable)
{
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	status = spi_flash_supports_address_mode (flash, enable);
	if (status != 0) {
		if (status == SPI_FLASH_ADDR_MODE_FIXED) {
			status = 0;
		}
		goto exit;
	}

	if (enable) {
		flash->addr_mode = FLASH_FLAG_4BYTE_ADDRESS;
	}
	else {
		flash->addr_mode = 0;
	}

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Enable and disable support for Quad SPI commands to the flash device.
 *
 * @param flash The flash to configure.
 * @param enable 1 to enable Quad commands or 0 to disable them.
 *
 * @return 0 if the command was successful or an error code.
 */
int spi_flash_enable_quad_spi (struct spi_flash *flash, uint8_t enable)
{
	struct flash_xfer xfer;
	uint8_t reg[2];
	uint8_t cmd_len = 2;
	uint8_t cmd = FLASH_CMD_RDSR;
	bool volatile_wren;
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	switch (flash->quad_enable) {
		case SPI_FLASH_SFDP_QUAD_NO_QE_BIT:
			status = 0;
			goto exit;

		case SPI_FLASH_SFDP_QUAD_NO_QE_HOLD_DISABLE:
			cmd = FLASH_CMD_RD_NV_CFG;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35:
			FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR2, &reg[1], 1, 0);
			status = flash->spi->xfer (flash->spi, &xfer);
			if (status != 0) {
				goto exit;
			}

			cmd_len = 1;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT6_SR1:
			cmd_len = 1;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT7_SR2:
			cmd_len = 1;
			cmd = FLASH_CMD_ALT_RDSR2;
			break;

		default:
			break;
	}

	FLASH_XFER_INIT_READ_REG (xfer, cmd, reg, cmd_len, 0);
	status = flash->spi->xfer (flash->spi, &xfer);
	if (status != 0) {
		goto exit;
	}

	cmd = FLASH_CMD_WRSR;
	volatile_wren = flash->sr1_volatile;

	switch (flash->quad_enable) {
		case SPI_FLASH_SFDP_QUAD_NO_QE_HOLD_DISABLE:
			if (enable) {
				reg[0] &= ~RESET_HOLD_ENABLE;
			}
			else {
				reg[0] |= RESET_HOLD_ENABLE;
			}

			cmd = FLASH_CMD_WR_NV_CFG;
			volatile_wren = false;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2:
		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_NO_CLR:
		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35:
			if (enable) {
				reg[1] |= QSPI_ENABLE_BIT1;
			}
			else {
				reg[1] &= ~QSPI_ENABLE_BIT1;
			}

			cmd_len = 2;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT6_SR1:
			if (enable) {
				reg[0] |= QSPI_ENABLE_BIT6;
			}
			else {
				reg[0] &= ~QSPI_ENABLE_BIT6;
			}
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT7_SR2:
			if (enable) {
				reg[0] |= QSPI_ENABLE_BIT7;
			}
			else {
				reg[0] &= ~QSPI_ENABLE_BIT7;
			}

			cmd = FLASH_CMD_ALT_WRSR2;
			volatile_wren = false;
			break;

		default:
			break;
	}

	status = spi_flash_write_register (flash, cmd, reg, cmd_len, volatile_wren);

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Determine if the SPI flash has Quad SPI enabled or disabled.
 *
 * @param flash The flash device to query.
 *
 * @return 0 if Quad SPI is disabled, 1 if Quad SPI is enabled, or an error code.
 */
int spi_flash_is_quad_spi_enabled (struct spi_flash *flash)
{
	struct flash_xfer xfer;
	uint8_t reg[2];
	uint8_t cmd_len = 2;
	uint8_t cmd = FLASH_CMD_RDSR;
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	switch (flash->quad_enable) {
		case SPI_FLASH_SFDP_QUAD_NO_QE_BIT:
			status = 1;
			goto exit;

		case SPI_FLASH_SFDP_QUAD_NO_QE_HOLD_DISABLE:
			cmd = FLASH_CMD_RD_NV_CFG;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35:
			cmd = FLASH_CMD_RDSR2;
			cmd_len = 1;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT6_SR1:
			cmd_len = 1;
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT7_SR2:
			cmd = FLASH_CMD_ALT_RDSR2;
			cmd_len = 1;
			break;

		default:
			break;
	}

	FLASH_XFER_INIT_READ_REG (xfer, cmd, reg, cmd_len, 0);
	status = flash->spi->xfer (flash->spi, &xfer);
	if (status != 0) {
		goto exit;
	}

	switch (flash->quad_enable) {
		case SPI_FLASH_SFDP_QUAD_NO_QE_HOLD_DISABLE:
			status = !(reg[0] & RESET_HOLD_ENABLE);
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35:
			reg[1] = reg[0];
			/* fall through */ /* no break */

		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2:
		case SPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_NO_CLR:
			status = !!(reg[1] & QSPI_ENABLE_BIT1);
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT6_SR1:
			status = !!(reg[0] & QSPI_ENABLE_BIT6);
			break;

		case SPI_FLASH_SFDP_QUAD_QE_BIT7_SR2:
			status = !!(reg[0] & QSPI_ENABLE_BIT7);
			break;

		default:
			break;
	}

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
* Configure the output drive strength of the flash device, if necessary.  If no drive strength
* configuration is needed for the device, nothing is done.
*
* @param flash The flash device to configure.
*
* @return 0 if the drive strength was successfully configured or an error code.
*/
int spi_flash_configure_drive_strength (struct spi_flash *flash)
{
	struct flash_xfer xfer;
	uint8_t vendor;
	uint8_t reg;
	uint8_t data = 0x20;	// 75% drive strength
	int status = 0;

	status = spi_flash_get_device_id (flash, &vendor, NULL);
	if (status != 0) {
		return status;
	}

	platform_mutex_lock (&flash->lock);

	switch (vendor) {
		case FLASH_ID_WINBOND:
			FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR3, &reg, 1, 0);
			status = flash->spi->xfer (flash->spi, &xfer);
			if (status != 0) {
				break;
			}

			if (data != (reg & 0x60)) {
				data |= (reg & ~0x60);
				status = spi_flash_write_register (flash, 0x11, &data, 1, false);
				if (status != 0) {
					break;
				}

				FLASH_XFER_INIT_READ_REG (xfer, FLASH_CMD_RDSR3, &reg, 1, 0);
				status = flash->spi->xfer (flash->spi, &xfer);
				if (status != 0) {
					break;
				}

				if (reg != data) {
					status = SPI_FLASH_CONFIG_FAILURE;
				}
			}
			break;
	}

	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Read data from the SPI flash.
 *
 * @param flash The flash to read from.
 * @param address The address to start reading from.
 * @param data The buffer to hold the data that has been read.
 * @param length The number of bytes to read.
 *
 * @return 0 if the bytes were read from flash or an error code.
 */
int spi_flash_read (struct spi_flash *flash, uint32_t address, uint8_t *data, size_t length)
{
	struct flash_xfer xfer;
	int status;

	if ((flash == NULL) || (data == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	SPI_FLASH_BOUNDS_CHECK (flash->device_size, address, length)

	platform_mutex_lock (&flash->lock);

	status = spi_flash_is_wip_set (flash);
	if (status != 0) {
		status = (status == 1) ? SPI_FLASH_WRITE_IN_PROGRESS : status;
		goto exit;
	}

	FLASH_XFER_INIT_READ (xfer, flash->command.read, address, flash->command.read_dummy,
		flash->command.read_mode, data, length, flash->command.read_flags | flash->addr_mode);
	status = flash->spi->xfer (flash->spi, &xfer);

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Get the size of a flash page for write operations.
 *
 * @param flash The flash to query.
 * @param bytes Output for the number of bytes in a flash page.
 *
 * @return 0 if the page size was successfully read or an error code.
 */
int spi_flash_get_page_size (struct spi_flash *flash, uint32_t *bytes)
{
	if ((flash == NULL) || (bytes == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	/* All supported devices use a 256 byte page size.  If necessary, this value can be read from
	 * the SFDP tables. */
	*bytes = FLASH_PAGE_SIZE;
	return 0;
}

/**
 * Get the minimum number of bytes that must be written to a single flash page.  Writing fewer bytes
 * than the minimum to any page will still result in a minimum sized write to flash. The extra bytes
 * that were written must be erased before they can be written again.
 *
 * @param flash The flash to query.
 * @param bytes Output for the minimum number of bytes for a page write.
 *
 * @return 0 if the minimum write size was successfully read or an error code.
 */
int spi_flash_minimum_write_per_page (struct spi_flash *flash, uint32_t *bytes)
{
	if ((flash == NULL) || (bytes == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	*bytes = 1;
	return 0;
}

/**
 * Write data to the SPI flash.  The flash needs to be erased prior to writing.
 *
 * @param flash The flash to write to.
 * @param address The address to start writing to.
 * @param data The data to write.
 * @param length The number of bytes to write.
 *
 * @return The number of bytes written to the flash or an error code.  Use ROT_IS_ERROR to check the
 * return value.
 */
int spi_flash_write (struct spi_flash *flash, uint32_t address, const uint8_t *data, size_t length)
{
	struct flash_xfer xfer;
	uint32_t page = FLASH_PAGE_BASE (address);
	uint32_t next = page + FLASH_PAGE_SIZE;
	size_t remaining = length;
	int status = 0;

	if ((flash == NULL) || (data == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	SPI_FLASH_BOUNDS_CHECK (flash->device_size, address, length);

	platform_mutex_lock (&flash->lock);

	status = spi_flash_is_wip_set (flash);
	if (status != 0) {
		status = (status == 1) ? SPI_FLASH_WRITE_IN_PROGRESS : status;
		goto exit;
	}

	while ((status == 0) && remaining) {
		uint32_t end = address + remaining;
		size_t write_len;

		if (page != FLASH_PAGE_BASE (end)) {
			write_len = next - address;
		}
		else {
			write_len = remaining;
		}

		status = spi_flash_write_enable (flash);
		if (status != 0) {
			continue;
		}

		FLASH_XFER_INIT_WRITE (xfer, flash->command.write, address, 0, (uint8_t*) data, write_len,
			flash->command.write_flags | flash->addr_mode);

		status = flash->spi->xfer (flash->spi, &xfer);
		if (status == 0) {
			status = spi_flash_wait_for_write_completion (flash, -1, 1);
			if (status == 0) {
				remaining -= write_len;
				data += write_len;
				page = next;
				address = next;
				next += FLASH_PAGE_SIZE;
			}
		}
	}

exit:
	platform_mutex_unlock (&flash->lock);

	length = length - remaining;
	if (length) {
		if (status != 0) {
			debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_FLASH,
				FLASH_LOGGING_INCOMPLETE_WRITE, address, status);
		}
		return length;
	}
	else {
		return status;
	}
}

/**
 * Erase a region of flash.
 *
 * @param flash The flash to erase.
 * @param address An address within the region to erase.
 * @param erase_cmd The erase command to use.
 * @param erase_flags Transfer flags for the command.
 *
 * @return 0 if the region was erased or an error code.
 */
static int spi_flash_erase_region (struct spi_flash *flash, uint32_t address, uint8_t erase_cmd,
	uint16_t erase_flags)
{
	struct flash_xfer xfer;
	int status;

	if (address >= flash->device_size) {
		return SPI_FLASH_ADDRESS_OUT_OF_RANGE;
	}

	platform_mutex_lock (&flash->lock);

	status = spi_flash_is_wip_set (flash);
	if (status != 0) {
		status = (status == 1) ? SPI_FLASH_WRITE_IN_PROGRESS : status;
		goto exit;
	}

	status = spi_flash_write_enable (flash);
	if (status != 0) {
		goto exit;
	}

	FLASH_XFER_INIT_NO_DATA (xfer, erase_cmd, address, erase_flags | flash->addr_mode);

	status = flash->spi->xfer (flash->spi, &xfer);
	if (status != 0) {
		goto exit;
	}

	status = spi_flash_wait_for_write_completion (flash, -1, 0);

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Get the size of a flash sector for erase operations.
 *
 * @param flash The flash to query.
 * @param bytes Output for the number of bytes in a flash sector.
 *
 * @return 0 if the sector size was successfully read or an error code.
 */
int spi_flash_get_sector_size (struct spi_flash *flash, uint32_t *bytes)
{
	if ((flash == NULL) || (bytes == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	/* It is possible to detect this value through SFDP.  As more flash devices are supported, it
	 * may be necessary to parse this value from the SFDP tables. */
	*bytes = FLASH_SECTOR_SIZE;
	return 0;
}

/**
 * Erase a 4kB sector of flash.
 *
 * @param flash The flash to erase.
 * @param sector_addr An address within the sector to erase.
 *
 * @return 0 if the sector was erased or an error code.
 */
int spi_flash_sector_erase (struct spi_flash *flash, uint32_t sector_addr)
{
	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	return spi_flash_erase_region (flash, FLASH_SECTOR_BASE (sector_addr),
		flash->command.erase_sector, flash->command.sector_flags);
}

/**
 * Get the size of a flash block for erase operations.
 *
 * @param flash The flash to query.
 * @param bytes Output for the number of bytes in a flash block.
 *
 * @return 0 if the block size was successfully read or an error code.
 */
int spi_flash_get_block_size (struct spi_flash *flash, uint32_t *bytes)
{
	if ((flash == NULL) || (bytes == NULL)) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	/* It is possible to detect this value through SFDP.  As more flash devices are supported, it
	 * may be necessary to parse this value from the SFDP tables. */
	*bytes = FLASH_BLOCK_SIZE;
	return 0;
}

/**
 * Erase a 64kB block of flash.
 *
 * @param flash The flash to erase.
 * @param block_addr An address within the block to erase.
 *
 * @return 0 if the block was erased or an error code.
 */
int spi_flash_block_erase (struct spi_flash *flash, uint32_t block_addr)
{
	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	return spi_flash_erase_region (flash, FLASH_BLOCK_BASE (block_addr), flash->command.erase_block,
		flash->command.block_flags);
}

/**
 * Erase the entire flash chip.
 *
 * @param flash The flash to erase.
 *
 * @return 0 if the flash chip was erased or an error code.
 */
int spi_flash_chip_erase (struct spi_flash *flash)
{
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);

	status = spi_flash_is_wip_set (flash);
	if (status != 0) {
		status = (status == 1) ? SPI_FLASH_WRITE_IN_PROGRESS : status;
		goto exit;
	}

	status = spi_flash_write_enable (flash);
	if (status != 0) {
		goto exit;
	}

	status = spi_flash_simple_command (flash, FLASH_CMD_CE);
	if (status != 0) {
		goto exit;
	}

	status = spi_flash_wait_for_write_completion (flash, -1, 0);

exit:
	platform_mutex_unlock (&flash->lock);
	return status;
}

/**
 * Determine if the flash is currently executing a write command.
 *
 * @param flash The flash instance to check.
 *
 * @return 0 if no write is in progress, 1 if there is, or an error code.
 */
int spi_flash_is_write_in_progress (struct spi_flash *flash)
{
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);
	status = spi_flash_is_wip_set (flash);
	platform_mutex_unlock (&flash->lock);

	return status;
}

/**
 * Wait for a write operation to complete.
 *
 * @param flash The flash instance that is executing a write operation.
 * @param timeout The maximum number of milliseconds to wait for completion.  A negative number will
 * wait forever.  0 will return immediately.
 *
 * @return 0 if the write was completed or an error code.
 */
int spi_flash_wait_for_write (struct spi_flash *flash, int32_t timeout)
{
	int status;

	if (flash == NULL) {
		return SPI_FLASH_INVALID_ARGUMENT;
	}

	platform_mutex_lock (&flash->lock);
	status = spi_flash_wait_for_write_completion (flash, timeout, 0);
	platform_mutex_unlock (&flash->lock);

	return status;
}
