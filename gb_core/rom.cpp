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

//-----------------------------------------------
// ROMイメージ管理部 (含SRAM) // ROM image management unit (SRAM included)

#include "gb.h"
#include <stdlib.h>
#include <string.h>

rom::rom()
{
	b_loaded     = false;
   b_persistent = false;

	dat          = NULL;
	sram         = NULL;
}

rom::~rom()
{
   if (!b_persistent)
      free(dat);
	free(sram);
}

bool rom::has_battery()
{
	static const int has_bat[]=
		{0,0,0,1,0, 0,1,0,0,1,
		 0,0,1,1,0, 1,1,0,0,1,
		 0,0,0,0,0, 0,0,1,0,1,
		 1,0, 0,0,0,0,0,0,0,0}; // 0x20以下
	return has_bat[(info.cart_type>0x20)?3:info.cart_type]==1;
}

int rom::get_sram_size()
{
	static const int tbl_ram[]={1,1,1,4,16,8};//0と1は保険
	// Header byte 0x149 is attacker-controlled in malformed/fuzzed
	// ROMs; values >=6 used to read past tbl_ram. Treat as "no SRAM".
	if (info.ram_size >= (int)(sizeof(tbl_ram)/sizeof(tbl_ram[0])))
		return 0;
	return 0x2000*tbl_ram[info.ram_size];
}

bool rom::load_rom(byte *buf,int size,byte *ram,int ram_size, bool persistent)
{
	byte momocol_title[16]={0x4D,0x4F,0x4D,0x4F,0x43,0x4F,0x4C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	// Reject anything that can't even contain the header.
	if (size < 0x150)
		return false;

	if (b_loaded){
      // Free-decision must key off the *previous* call's flag
      // (b_persistent), not the new `persistent` arg. Otherwise
      // persistent->non-persistent transitions free() a frontend-
      // owned buffer (UAF), and non-persistent->persistent leak
      // our previous malloc.
      if (!b_persistent)
         free(dat);
		free(sram);
		dat  = NULL;
		sram = NULL;
	}

	memcpy(info.cart_name,buf+0x134,16);
	info.cart_name[16]='\0';
	info.cart_name[17]='\0';
	info.cart_type=buf[0x147];
	info.rom_size=buf[0x148];
	info.ram_size=buf[0x149];

	if (memcmp(info.cart_name,momocol_title,16)==0){
		info.cart_type=0x100;//mmm01
	}

	byte tmp2=buf[0x143];

	info.gb_type=(tmp2&0x80)?3:1;

	if (info.rom_size>8)
		return false;
	// header-declared ROM size must fit in the buffer we got
	{
		static const int rom_banks_tbl[9]={2,4,8,16,32,64,128,256,512};
		int declared = rom_banks_tbl[info.rom_size]*0x4000;
		if (declared > size)
			return false;
	}
	// ram_size header byte is also a table index in mbc.cpp; reject
	// rather than read past tbl_ram later.
	if (info.ram_size >= 6)
		return false;

   if (persistent)
      dat = (byte*)buf;
   else
   {
      dat=(byte*)malloc(size);
      if (!dat)
         return false;
      memcpy(dat,buf,size);
   }
	first_page=dat;

	int sram_bytes = get_sram_size();
	if (sram_bytes > 0) {
		sram=(byte*)malloc(sram_bytes);
		if (!sram) {
			if (!persistent) free(dat);
			dat = NULL;
			return false;
		}
		// Real cartridges power on with 0xFF, not random heap bytes.
		// Without this memset, uninitialized memory leaks out via
		// RETRO_MEMORY_SAVE_RAM and breaks save-state determinism.
		memset(sram, 0xFF, sram_bytes);
		if (ram) {
			int copy = (ram_size < sram_bytes) ? ram_size : sram_bytes;
			if (copy > 0)
				memcpy(sram, ram, copy);
		}
	} else {
		sram = NULL;
	}

	b_loaded     = true;
   b_persistent = persistent;

	return true;
}

void rom::serialize(serializer &s)
{
	s_VAR(info);
	int sram_bytes = get_sram_size();
	if (sram_bytes > 0)
		s.process(sram, sram_bytes);
}

