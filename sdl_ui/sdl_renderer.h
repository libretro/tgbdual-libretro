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

#include "../gb_core/renderer.h"
#include <stdio.h>
#include <vector>

#include "w32_sdl.h"

#include <SDL.h>

using namespace std;

///#define DIRECTDRAW_VERSION 0x0300
///#define DIRECTSOUND_VERSION 0x0300
///#define DIRECTINPUT_VERSION 0x0500

///#include "wavwrite.h"

///#include "sock.h"

#define DI_KEYBOARD 1
#define DI_MOUSE_X 2
#define DI_MOUSE_Y 3
#define DI_MOUSE 4
#define DI_PAD_X 5
#define DI_PAD_Y 6
#define DI_PAD 7
#define NEXT_PAD 3

struct key_dat{
	int device_type;
	int key_code;
};

struct mov_key{
	int frame;
	int key_code;
};

struct col_filter{
	int r_def,g_def,b_def;
	int r_div,g_div,b_div;
	int r_r,r_g,r_b;
	int g_r,g_g,g_b;
	int b_r,b_g,b_b;
};

class sdl_renderer : public renderer
{
public:
	sdl_renderer();
	~sdl_renderer();

	void render_screen(byte *buf,int width,int height,int depth);
	int check_pad();
	void set_pad(int stat);
	void refresh();
	void output_log(char *mes,...);
	void reset() {}
	word map_color(word gb_col);
	word unmap_color(word gb_col);
	byte get_time(int type);
	void set_time(int type,byte dat);

	void swith_screen_mode();
	void set_render_pass(int type);
	bool get_screen_mode() { return b_window; }
	void set_mul(int num) { mul=num; }
	void set_key(key_dat *keys);
	void set_koro_key(key_dat *keys);
	void set_koro_analog(bool ana);
	void set_koro_sensitivity(int sence);
	void flip();
	void on_move();
	void draw_menu(int n);
	void show_fps(bool b_show) { b_showfps=b_show; }
	void show_message(const char *message);
	word get_any_key();

	int get_timer_state();
	void set_timer_state(int timer);

	word get_sensor(bool x_y);
	void set_bibrate(bool bibrate);

	void set_filter(col_filter *fil) { m_filter=*fil; };

	void set_save_key(key_dat *key_code) { save_key=*key_code; }
	void set_load_key(key_dat *key_code) { load_key=*key_code; }
	void set_auto_key(key_dat *key_code) { auto_key=*key_code; }
	void set_pause_key(key_dat *key_code) { pause_key=*key_code; }
	void set_full_key(key_dat *key_code) { full_key=*key_code; }
	void set_reset_key(key_dat *key_code) { reset_key=*key_code; }
	void set_quit_key(key_dat *key_code) { quit_key=*key_code; }

	void set_save_resurve(int slot) { save_resurve=slot; }
	void set_load_resurve(int slot) { load_resurve=slot; }

	bool check_press(key_dat *dat);

	void pause_sound();
	void resume_sound();

	void movie_record_start(FILE *file) { movie_file=file; mov_cur_pos=0; movie_start=0; movie_recording=true; }
	void movie_record_stop() { int tmp=movie_start+1; fwrite(&tmp,4,1,movie_file); tmp=0xffffffff; fwrite(&tmp,4,1,movie_file); movie_recording=false; }
	void movie_play_start(vector<mov_key> *list);
	void movie_play_stop() { movie_playing=false; key_list.clear(); mov_cur_pos=0; }

	void update_pad();
	void disable_check_pad();
	void enable_check_pad();
	void toggle_auto();
	void set_use_ffb(bool use);

	void graphics_record(char *file);
	void sound_record(char *file);

private:
	//--------------------------------

	void init_sdlvideo();
	void uninit_sdlvideo();
	void init_surface();
	void release_surface();

/*
	LPDIRECTDRAW m_pdd;
	LPDIRECTDRAWSURFACE m_pps;
	LPDIRECTDRAWSURFACE m_pbs;
	LPDIRECTDRAWSURFACE m_pss;
	LPDIRECTDRAWSURFACE m_pss2;
	LPDIRECTDRAWCLIPPER m_pclip;

	RECT m_window;
	RECT m_viewport;
	RECT m_screen;
	RECT m_bkup;
*/

	int width;
	int height;
	int depth;
	int bpp;

	int color_type;
	bool b_640_480;

	bool b_window;
	bool b_showfps;
	int fps;
	int mul;
	char mes[128];
	bool mes_show;
///	DWORD mes_start;

	int render_pass_type; // 0 Heap->Sys->Primary 1 Heap->VRAM->Primary 2 Heap->Sys->VRAM->VRAM

	col_filter m_filter;

	DWORD map_24[0x10000];

	//------------------------------

	void init_sdlaudio();
	void uninit_sdlaudio();

/*
	LPDIRECTSOUND m_pds;
	LPDIRECTSOUNDBUFFER m_ppb;
	LPDIRECTSOUNDBUFFER m_pmix;
*/

	//------------------------------

	void init_sdlevent();
	void uninit_sdlevent();

///	static BOOL CALLBACK pad_callback(LPCDIDEVICEINSTANCE pdinst,LPVOID pvRef);

/*
	LPDIRECTINPUT m_pdi;
	LPDIRECTINPUTDEVICE m_pkeyboad;
	LPDIRECTINPUTDEVICE m_pmouse;
	LPDIRECTINPUTDEVICE m_pjoystick[16];
	LPDIRECTINPUTDEVICE2 m_pjoystick2[16];
	LPDIRECTINPUTEFFECT m_peffect;
	POINT joy_center[16];
*/

	int joysticks;
	int pad_state;

	key_dat key_config[8],koro_key_config[4];
	bool b_koro_analog;
	int koro_sence;
	bool b_bibrating;
	bool b_can_use_ffb;
	bool b_use_ffb;
///	DIJOYSTATE js[16];

	int now_sensor_x,now_sensor_y;
	bool b_pad_update;
	bool b_auto;

	key_dat save_key,load_key,auto_key,pause_key,full_key,reset_key,quit_key;

	vector<mov_key> key_list;
	bool movie_playing,movie_recording;
	FILE *movie_file;
	int movie_start,mov_cur_pos;

	//-------------------------------

	//-------------------------------
	int save_resurve;
	int load_resurve;

	bool b_graphics;
	bool b_sound;

///	CWaveSoundWrite *wav;
	int snd_size;
	char graphics_file[256];

	int cur_time;

///	HWND m_hwnd;
///	HINSTANCE m_hinst;

private:
	// dpy が NULL なら scr が screen
	SDL_Surface* dpy;
	SDL_Surface* scr;

};
