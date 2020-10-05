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
extern retro_environment_t environ_cb;

extern bool gblink_enable;

extern int audio_2p_mode;

#define MSG_FRAMES 60
#define SAMPLES_PER_FRAME (44100/60)

bool _screen_2p_vertical = false;
bool _screen_switched = false; // set to draw player 2 on the left/top
int _show_player_screens = 2; // 0 = p1 only, 1 = p2 only, 2 = both players

static inline bool button_pressed(int pad, int btn)
{
   static bool held[16] = {
      false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false,
   };
   if ( input_state_cb(pad, 1,0, btn) )
   {
      if ( ! held[btn] )
      {
         held[btn] = true;
         return true;
      }
   }
   else
      held[btn] = false;
   return false;
}

dmy_renderer::dmy_renderer(int which)
{
	which_gb = which;

	retro_pixel_format pixfmt = RETRO_PIXEL_FORMAT_RGB565;
	rgb565 = environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixfmt);

#ifndef FRONTEND_SUPPORTS_RGB565
	if (rgb565)
		puts("Frontend supports RGB565; will use that instead of XRGB1555.");
#endif
}

word dmy_renderer::map_color(word gb_col)
{
#ifndef SKIP_COLOR_CORRECTION
#ifndef FRONTEND_SUPPORTS_RGB565
   if(rgb565)
   {
#endif
      return ((gb_col&0x001f) << 11) |
         ((gb_col&0x03e0) <<  1) |
         ((gb_col&0x0200) >>  4) |
         ((gb_col&0x7c00) >> 10);
#ifndef FRONTEND_SUPPORTS_RGB565
   }
   return ((gb_col&0x001f) << 10) |
      ((gb_col&0x03e0)      ) | 
      ((gb_col&0x7c00) >> 10);
#endif
#else
   return gb_col;
#endif
}

word dmy_renderer::unmap_color(word gb_col)
{
#ifndef SKIP_COLOR_CORRECTION
#ifndef FRONTEND_SUPPORTS_RGB565
   if(rgb565)
   {
#endif
      return ((gb_col&0x001f) << 10) |
         ((gb_col&0x07c0) >>  1) |
         ((gb_col&0xf800) >> 11);
#ifndef FRONTEND_SUPPORTS_RGB565
   }
   return ((gb_col&0x001f) << 10) |
      ((gb_col&0x03e0)      ) | 
      ((gb_col&0x7c00) >> 10);
#endif
#else
   return gb_col;
#endif
}

void dmy_renderer::refresh() {
	static int16_t stream[SAMPLES_PER_FRAME*2];

   if (g_gb[1] && gblink_enable)
   {
      // if dual gb mode
      if (audio_2p_mode == 2)
      {
         // mix down to one per channel (dual mono)
         int16_t tmp_stream[SAMPLES_PER_FRAME*2];
         this->snd_render->render(tmp_stream, SAMPLES_PER_FRAME);
         for(int i = 0; i < SAMPLES_PER_FRAME; ++i)
         {
            int l = tmp_stream[(i*2)+0], r = tmp_stream[(i*2)+1];
            stream[(i*2)+which_gb] = int16_t( (l+r) / 2 );
         }
      }
      else if (audio_2p_mode == which_gb)
      {
         // only play gb 0 or 1
         this->snd_render->render(stream, SAMPLES_PER_FRAME);
      }
      if (which_gb == 1)
      {
         // only do audio callback after both gb's are rendered.
         audio_batch_cb(stream, SAMPLES_PER_FRAME);

         /*
         // switch screen orientation with X button
         // this isn't enough to make retroarch change resolutions on the fly,
         // even if the values returned by get_system_av_info change as well.
         if ( button_pressed(0, RETRO_DEVICE_ID_JOYPAD_X) ) {
         _screen_2p_vertical = ! _screen_2p_vertical;
         }*/
         audio_2p_mode &= 3;
         memset(stream, 0, sizeof(stream));
      }
   }
   else
   {
		this->snd_render->render(stream, SAMPLES_PER_FRAME);
		audio_batch_cb(stream, SAMPLES_PER_FRAME);
	}
	fixed_time = time(NULL);
}

int dmy_renderer::check_pad()
{
	// update pad state: a,b,select,start,down,up,left,right
	pad_state =
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_A)     ) << 0 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_B)     ) << 1 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_SELECT)) << 2 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_START) ) << 3 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_DOWN)  ) << 4 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_UP)    ) << 5 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_LEFT)  ) << 6 |
	(!!input_state_cb(which_gb,1,0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ) << 7;
	return pad_state;
}

void dmy_renderer::render_screen(byte *buf,int width,int height,int depth)
{
   static byte joined_buf[160*144*2*2]; // two screens' worth of 16-bit data
   const int half = sizeof(joined_buf)/2;
   int pitch = width*((depth+7)/8);
   int switched_gb = which_gb;
   if (_screen_switched)
      switched_gb = 1 - switched_gb;

   // are we running two gb's?
   if(g_gb[1] && gblink_enable)
   {
      // are we drawing both gb's to the screen?
      if (_show_player_screens == 2)
      {
         if(_screen_2p_vertical)
         {
            memcpy(joined_buf + switched_gb*half, buf, half);
            if(which_gb == 1)
               video_cb(joined_buf, width, height*2, pitch);
         }
         else
         {
            for (int row = 0; row < height; ++row)
               memcpy(joined_buf + pitch*(2*row + switched_gb), buf+pitch*row, pitch);
            if(which_gb == 1)
               video_cb(joined_buf, width*2, height, pitch*2);
         }
      }
      else
      {
         // are we currently on the gb that we want to draw?
         // (this ignores the "switch player screens" setting)
         if (_show_player_screens == which_gb)
            memcpy(joined_buf, buf, half);
         if (which_gb == 1)
            video_cb(joined_buf, width, height, pitch);
      }
   }
   else
      video_cb(buf, width, height, pitch);
}

byte dmy_renderer::get_time(int type)
{
   dword now = fixed_time-cur_time;

   switch(type)
   {
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
   dword now = fixed_time;
   dword adj = now - cur_time;

   switch(type)
   {
      case 8: // second
         adj = (adj/60)*60+(dat%60);
         break;
      case 9: // minute
         adj = (adj/(60*60))*60*60+(dat%60)*60+(adj%60);
         break;
      case 10: // hour
         adj = (adj/(24*60*60))*24*60*60+(dat%24)*60*60+(adj%(60*60));
         break;
      case 11: // day (L)
         adj = (adj/(256*24*60*60))*256*24*60*60+(dat*24*60*60)+(adj%(24*60*60));
         break;
      case 12: // day (H)
         adj = (dat&1)*256*24*60*60+(adj%(256*24*60*60));
         break;
   }
   cur_time = now - adj;
}

