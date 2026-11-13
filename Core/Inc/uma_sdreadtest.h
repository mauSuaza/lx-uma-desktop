/*
 * uma_sdreadtest.h - LX_UMA test 0.1a (docs/phase1-spec.md, section 3, option A): how long does reading one kit's
 * UMA settings from the card take while a pattern plays? Behind UMA_SDREAD_TEST (uma.h).
 */
#ifndef UMA_SDREADTEST_H_
#define UMA_SDREADTEST_H_
#include <stdint.h>

void umaTest_sdReadTick(uint8_t shown);	// main loop; reads twice a second while the readout is shown
uint8_t umaTest_sdReadTime(void);		// the longest read in the last 2 s, in 0.1 ms (255 = 25.5 ms or more)

#endif /* UMA_SDREADTEST_H_ */
