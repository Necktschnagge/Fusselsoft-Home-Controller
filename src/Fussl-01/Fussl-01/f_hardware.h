/*
* f_hardware.h
*
* Created: 01.09.2016 15:10:01
*  Author: Maximilian Starke
*/
/************************************************************************/
/* FPS: nothing to do, small functions
<<< some day: please look at the declaration of _delay_ms in cpp
and checkout whether it is possible to pipe higher arguments than 10 ms.
*/
/************************************************************************/

/************************************************************************/
/* hardware contains some utilities often required
you can just use them without anything to prepare
*/
/************************************************************************/


#include <stdint.h>

#ifndef F_HARDWARE_H_
#define F_HARDWARE_H_


namespace hardware {

	constexpr uint16_t EEPNULL {0xFFFF}; // nullptr for the EEPROM
	
	/* make a busy waiting time gap [ms] */
	void delay(uint16_t ms);
	
	/* return whether an eeprom address is out of range (NULL), throw an error if a non standard NULL address (!=EEPNULL) was used */
	bool isEEPNull(uint16_t address);

	/* copy a string from source to destination */
	/* it will only copy (count) characters and will add an '\0' if nullTerminated */
	/* the real string length of destination will be count + 1 (if nullTerminated) */
	/* but if source has an '\0' before copying will be stopped immediately */
	void copyString(char* destination, const char* source, uint8_t count, bool nullTerminated);
	
	template <typename T>
	inline void busy_wait(T range){
		for (T i = 0; i < range; ++i){ /* do nothing */ }
	}

}

#endif /* F_HARDWARE_H_ */