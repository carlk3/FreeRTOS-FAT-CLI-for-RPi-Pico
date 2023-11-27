/* util.h
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

#pragma once

#include <RP2040.h>
#include <stddef.h>    
#include <stdint.h>

#include "hardware/structs/scb.h"
//
#include "my_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

// works with negative index
static inline int wrap_ix(int index, int n)
{
    return ((index % n) + n) % n;
}
static inline int mod_floor(int a, int n) {
    return ((a % n) + n) % n;
}

__attribute__((always_inline)) static inline uint32_t calculate_checksum(uint32_t const *p, size_t const size){
	uint32_t checksum = 0;
	for (uint32_t i = 0; i < (size/sizeof(uint32_t))-1; i++){
		checksum ^= *p;
		p++;
	}
	return checksum;
}


// from Google Chromium's codebase:
#ifndef COUNT_OF    
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#endif

// Patterned after CMSIS NVIC_SystemReset
__attribute__((__noreturn__)) static inline void system_reset() {
    __DSB(); /* Ensure all outstanding memory accesses included
         buffered write are completed before reset */
    scb_hw->aircr = ((0x5FAUL << 16U) | (1UL << 2U));
    __DSB(); /* Ensure completion of memory access */
    for (;;) {
        __asm volatile("nop");
    }
}

char const* uint_binary_str(unsigned int number);

static inline uint32_t ext_bits(size_t src_bytes, unsigned char const *data, int msb, int lsb) {
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = (src_bytes - 1) - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}
static inline uint32_t ext_bits16(unsigned char const *data, int msb, int lsb) {
    return ext_bits(16, data, msb, lsb);
}


#ifdef __cplusplus
}
#endif
/* [] END OF FILE */
