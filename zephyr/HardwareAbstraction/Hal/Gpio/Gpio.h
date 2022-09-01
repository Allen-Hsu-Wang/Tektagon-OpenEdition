//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the GPIO Handling functions
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
/**
 * Storage for device keys and certificates.
 */
struct GpioInterface {
	/**
	 * GPIO Set.
	 *
	 * @param Gpio Interface 
	 * @param id The ID of the key being stored.
	 *
	 * @return 0 if the key was successfully stored or an error code.
	 */
	int (*GpioSet) ();

	/**
	 * GPIO Set.
	 *
	 * @param store The key storage where the key is saved.
	 * @param id The ID of the key to load.
	 *
	 * @return 0 if the key was successfully loaded or an error code.
	 */
	int (*GpioGet) ();

	/**
	 * Boot Release
	 *
	 * @param Gpio Interface 
	 *
	 * @return 0 if the successfully erased or an error code.
	 */
	int (*SysReset) ();
};

int GpioInit (struct GpioInterface *Gpio);

/**
 * Error codes that can be generated by a GPIO.
 */
enum {
	GPIO_INVALID_ARGUMENT = 0x00		/**< Input parameter is null or not valid. */
};

#endif /* GPIO_H_ */