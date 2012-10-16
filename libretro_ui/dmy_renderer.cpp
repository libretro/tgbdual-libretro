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

// libretro implementation of the renderer, should probably be renamed from dmy.

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "dmy_renderer.h"
#include "../gb_core/gb.h"
#include "libretro.h"

extern gb *g_gb[2];

extern retro_video_refresh_t video_cb;
extern retro_audio_sample_batch_t audio_batch_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;

#define SAMPLES_PER_FRAME (44100/60)

dmy_renderer::dmy_renderer(int which)
{
	which_gb = which;
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
	fixed_time = time(NULL);
	static int16_t stream[SAMPLES_PER_FRAME*2];
	if (g_gb[1]) { // if dual gb mode, mix down to one per channel
		int16_t tmp_stream[SAMPLES_PER_FRAME*2];
		this->snd_render->render(tmp_stream, SAMPLES_PER_FRAME);
		for(int i = 0; i < SAMPLES_PER_FRAME; ++i) {
			int l = tmp_stream[(i*2)+0], r = tmp_stream[(i*2)+1];
			stream[(i*2)+which_gb] = int16_t( (l+r) / 2 );
		}
		if (which_gb == 1) {
			audio_batch_cb(stream, SAMPLES_PER_FRAME);
		}
	} else {
		this->snd_render->render(stream, SAMPLES_PER_FRAME);
		audio_batch_cb(stream, SAMPLES_PER_FRAME);
	}
	input_poll_cb();
}

int dmy_renderer::check_pad()
{
	// a,b,select,start,down,up,left,right
	return
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_A)     ) << 0 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_B)     ) << 1 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_SELECT)) << 2 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_START) ) << 3 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_DOWN)  ) << 4 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_UP)    ) << 5 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_LEFT)  ) << 6 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ) << 7;
}

void dmy_renderer::render_screen(byte *buf,int width,int height,int depth) {
	static byte joined_buf[160*144*2*2]; // two screens' worth of 16-bit data
	const int half = sizeof(joined_buf)/2;
	if(g_gb[1]) { // are we running two gb's?
		memcpy(joined_buf + which_gb*half, buf, half);
		if(which_gb == 1) {
			video_cb(joined_buf, width, height*2, width*((depth+7)/8));
		}
	} else {
		video_cb(buf, width, height, width*((depth+7)/8));
	}
}

byte dmy_renderer::get_time(int type)
{
	dword now=fixed_time-cur_time;

	switch(type){
	case 8: // second
		return (byte)(now%60);
	case 9: // minute
		return (byte)((now/60)%60);
	case 10: // hour
		return (byte)((now/(60*60))%24);
	case 11: // day (L)
		return (byte)((now/(24*60*60))&0xff);
	case 12: // day (H)
		return (byte)((now/(256*24*60*60))&1);
	}
	return 0;
}

void dmy_renderer::set_time(int type,byte dat)
{
	dword now=fixed_time;
	dword adj=now-cur_time;

	switch(type){
	case 8: // second
		adj=(adj/60)*60+(dat%60);
		break;
	case 9: // minute
		adj=(adj/(60*60))*60*60+(dat%60)*60+(adj%60);
		break;
	case 10: // hour
		adj=(adj/(24*60*60))*24*60*60+(dat%24)*60*60+(adj%(60*60));
		break;
	case 11: // day (L)
		adj=(adj/(256*24*60*60))*256*24*60*60+(dat*24*60*60)+(adj%(24*60*60));
		break;
	case 12: // day (H)
		adj=(dat&1)*256*24*60*60+(adj%(256*24*60*60));
		break;
	}
	cur_time=now-adj;
}

