/*
 * uma_s7test.h - LX_UMA Phase 0 test 0.0b: the sector-7 pattern check (uma_s7test.c)
 */
#ifndef UMA_S7TEST_H_
#define UMA_S7TEST_H_
#include <stdint.h>

#define UMA_S7TEST_ADDRESS 0x080C0000u	// start of flash sector 7
#define UMA_S7TEST_SIZE 4096u

// bytes in sector 7 that differ from the pattern: 0 = the bootloader wrote the whole pattern
uint16_t uma_s7check(void);

#endif /* UMA_S7TEST_H_ */
