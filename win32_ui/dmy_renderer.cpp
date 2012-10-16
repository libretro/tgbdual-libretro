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

#include "dmy_renderer.h"

dmy_renderer::dmy_renderer()
{
	key_state=0;
	cur_time=0;
}

dmy_renderer::~dmy_renderer()
{
}

word dmy_renderer::map_color(word gb_col)
{
	return gb_col;
}

word dmy_renderer::unmap_color(word gb_col)
{
	return gb_col;
}

void dmy_renderer::set_pad(int state)
{
	key_state=state;
}

int dmy_renderer::check_pad()
{
	return key_state;
}
byte dmy_renderer::get_time(int type)
{
	dword now=fixed_time-cur_time;

	switch(type){
	case 8: // 秒
		return (byte)(now%60);
	case 9: // 分
		return (byte)((now/60)%60);
	case 10: // 時
		return (byte)((now/(60*60))%24);
	case 11: // 日(L)
		return (byte)((now/(24*60*60))&0xff);
	case 12: // 日(H)
		return (byte)((now/(256*24*60*60))&1);
	}
	return 0;
}

void dmy_renderer::set_time(int type,byte dat)
{
	dword now=fixed_time;
	dword adj=now-cur_time;

	switch(type){
	case 8: // 秒
		adj=(adj/60)*60+(dat%60);
		break;
	case 9: // 分
		adj=(adj/(60*60))*60*60+(dat%60)*60+(adj%60);
		break;
	case 10: // 時
		adj=(adj/(24*60*60))*24*60*60+(dat%24)*60*60+(adj%(60*60));
		break;
	case 11: // 日(L)
		adj=(adj/(256*24*60*60))*256*24*60*60+(dat*24*60*60)+(adj%(24*60*60));
		break;
	case 12: // 日(H)
		adj=(dat&1)*256*24*60*60+(adj%(256*24*60*60));
		break;
	}
	cur_time=now-adj;
}

void dmy_renderer::set_fixed_time(dword time)
{
	fixed_time=time;
}
