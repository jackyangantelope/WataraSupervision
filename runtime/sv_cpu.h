/* SPDX-License-Identifier: BSD-3-Clause */
#pragma once
#include <stdint.h>
typedef uint8_t byte;
typedef uint16_t word;
byte Rd6502(word address);
void Wr6502(word address,byte value);
