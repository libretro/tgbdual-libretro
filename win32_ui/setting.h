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

class setting
{
public:
	setting();
	~setting();

	void get_key_setting(int *buf,int side);
	void set_key_setting(int *dat,int side);
	void get_cur_dir(char *buf);
	void set_cur_dir(char *dat);
	void get_save_dir(char *buf);
	void set_save_dir(char *dat);
	void get_media_dir(char *buf);
	void set_media_dir(char *dat);
	void get_dev_dir(char *buf);
	void get_home_dir(char *buf);

	bool speed_limit,fast_speed_limit;
	int frame_skip,virtual_fps;
	int fast_frame_skip,fast_virtual_fps;
	bool show_fps;

	int win_pos[4];
	bool vsync;
	int sound_enable[5]; // 4ch + master
	bool b_echo;
	bool b_lowpass;

	int save_key[2];
	int load_key[2];
	int auto_key[2];
	int fast_forwerd[2];
	int pause_key[2];

	char ip_addrs[4][20];

	bool koro_use_analog;
	int koro_sensitive;
	int koro_key[8];

	bool use_ffb;

	int r_def,g_def,b_def;
	int r_div,g_div,b_div;
	int r_r,r_g,r_b;
	int g_r,g_g,g_b;
	int b_r,b_g,b_b;

	int gb_type;
	bool use_gba;
	int render_pass;
	int priority_class;

private:
	int key_setting[2][16];
	char cur_dir[256],home_dir[256],save_dir[256],media_dir[256],dev_dir[256];
};
