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

// レンダラのダミー実装
// Dummy implementation of the renderer

#include <stdio.h>
#include <math.h>
#include "dmy_renderer.h"
#include "libretro.h"

extern retro_video_refresh_t video_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;

dmy_renderer::dmy_renderer()
{
}

dmy_renderer::~dmy_renderer()
{
}

word dmy_renderer::map_color(word gb_col)
{
	return ((gb_col&0x1F)<<10)|(gb_col&0x3e0)|((gb_col&0x7c00)>>10);
}

word dmy_renderer::unmap_color(word gb_col)
{
	return ((gb_col&0x1F)<<10)|(gb_col&0x3e0)|((gb_col&0x7c00)>>10);
}

void dmy_renderer::refresh() {
	input_poll_cb();
}

int dmy_renderer::check_pad()
{
	// a,b,select,start,down,up,left,right
	return
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_A)     ) << 0 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_B)     ) << 1 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_SELECT)) << 2 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_START) ) << 3 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_DOWN)  ) << 4 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_UP)    ) << 5 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_LEFT)  ) << 6 |
	(!!input_state_cb(0,1,0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ) << 7;
}

void dmy_renderer::render_screen(byte *buf,int width,int height,int depth) {
	//printf("video_cb(%x, %d, %d, %d)\n", buf, width, height, 256*(depth+7)/8);
	video_cb(buf, width, height, width*((depth+7)/8));
}

