/*--------------------------------------------------
   TGB Dual - Gameboy Emulator -
   Copyright (C) 2001  Hii

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------
// 型定義部

#ifndef _GB_TYPES
#define _GB_TYPES

#include <stdint.h>

// Fixed-width so save state layout is identical across platforms.
// Historically `dword` was `unsigned long`, which is 32-bit on
// Windows but 64-bit on Linux/macOS - making save states silently
// incompatible across hosts.
typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;

// -----------------------------------------------------------------
// Endianness
// -----------------------------------------------------------------
// Define MSB_FIRST when building for a big-endian host; otherwise
// little-endian is assumed (matches every libretro target we ship
// for in practice: x86, x86_64, ARM, AArch64, PPC64LE, RISC-V).
//
// LE16_LOAD(p) reads a little-endian 16-bit value from a byte
// pointer, regardless of host endianness. It is the safe way to
// read 16-bit values out of Game Boy VRAM (which has a hardware-
// fixed little-endian byte layout), because the legacy
// `*(word*)p` pattern only works when host endianness == LE.
//
// LE16_HOST_SWAP(w) converts a value loaded host-natively from
// VRAM back to the GB's logical 16-bit pixel-row layout.
// -----------------------------------------------------------------
static inline word LE16_LOAD(const byte *p) {
	return (word)((word)p[0] | ((word)p[1] << 8));
}

#ifdef MSB_FIRST
static inline word LE16_HOST_SWAP(word w) {
	return (word)((w >> 8) | (w << 8));
}
#else
static inline word LE16_HOST_SWAP(word w) { return w; }
#endif

#endif
