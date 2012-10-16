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

#include "setting.h"
#include <windows.h>
#include <stdio.h>
#define DIRECTINPUT_VERSION 0x0500
#include <dinput.h>

setting::setting()
{
	char ini_name[256];

	char *p,q[256],*r=NULL;
	p=GetCommandLine();

	// ファイル名抽出
	if (p[0]=='"'){
		strcpy(q,p+1);
		p=q;
		while(*(p++)!='"');
		*(p-1)='\0';
	}
	else{
		strcpy(q,p);
		p=q;
		while(*(p++)!=' ');
		*p='\0';
	}

	p=q;

	// ディレクトリ抽出
	for (int i=0;p[i]!='\0';i++)
		if (p[i]=='\\')
			r=p+i;

	if (r){
		*r='\0';
//		GetCurrentDirectory(256,home_dir);
		SetCurrentDirectory(q);
		strcpy(home_dir,q);
		strcpy(ini_name,home_dir);
		strcat(ini_name,"\\TGB.ini");
	}
	else{
		GetCurrentDirectory(256,home_dir);
		strcpy(ini_name,home_dir);
		strcat(ini_name,"\\TGB.ini");
	}

	// キー情報 (a,b,select,start,down,up,left,right の順)
	key_setting[0][0]=GetPrivateProfileInt("key_1","a_type",1,ini_name);
	key_setting[0][1]=GetPrivateProfileInt("key_1","a_code",DIK_Z,ini_name);
	key_setting[0][2]=GetPrivateProfileInt("key_1","b_type",1,ini_name);
	key_setting[0][3]=GetPrivateProfileInt("key_1","b_code",DIK_X,ini_name);
	key_setting[0][4]=GetPrivateProfileInt("key_1","select_type",1,ini_name);
	key_setting[0][5]=GetPrivateProfileInt("key_1","select_code",DIK_RSHIFT,ini_name);
	key_setting[0][6]=GetPrivateProfileInt("key_1","start_type",1,ini_name);
	key_setting[0][7]=GetPrivateProfileInt("key_1","start_code",DIK_RETURN,ini_name);
	key_setting[0][8]=GetPrivateProfileInt("key_1","down_type",1,ini_name);
	key_setting[0][9]=GetPrivateProfileInt("key_1","down_code",DIK_DOWN,ini_name);
	key_setting[0][10]=GetPrivateProfileInt("key_1","up_type",1,ini_name);
	key_setting[0][11]=GetPrivateProfileInt("key_1","up_code",DIK_UP,ini_name);
	key_setting[0][12]=GetPrivateProfileInt("key_1","left_type",1,ini_name);
	key_setting[0][13]=GetPrivateProfileInt("key_1","left_code",DIK_LEFT,ini_name);
	key_setting[0][14]=GetPrivateProfileInt("key_1","right_type",1,ini_name);
	key_setting[0][15]=GetPrivateProfileInt("key_1","right_code",DIK_RIGHT,ini_name);

	key_setting[1][0]=GetPrivateProfileInt("key_2","a_type",1,ini_name);
	key_setting[1][1]=GetPrivateProfileInt("key_2","a_code",DIK_DELETE,ini_name);
	key_setting[1][2]=GetPrivateProfileInt("key_2","b_type",1,ini_name);
	key_setting[1][3]=GetPrivateProfileInt("key_2","b_code",DIK_END,ini_name);
	key_setting[1][4]=GetPrivateProfileInt("key_2","select_type",1,ini_name);
	key_setting[1][5]=GetPrivateProfileInt("key_2","select_code",DIK_HOME,ini_name);
	key_setting[1][6]=GetPrivateProfileInt("key_2","start_type",1,ini_name);
	key_setting[1][7]=GetPrivateProfileInt("key_2","start_code",DIK_INSERT,ini_name);
	key_setting[1][8]=GetPrivateProfileInt("key_2","down_type",1,ini_name);
	key_setting[1][9]=GetPrivateProfileInt("key_2","down_code",DIK_NUMPAD2,ini_name);
	key_setting[1][10]=GetPrivateProfileInt("key_2","up_type",1,ini_name);
	key_setting[1][11]=GetPrivateProfileInt("key_2","up_code",DIK_NUMPAD8,ini_name);
	key_setting[1][12]=GetPrivateProfileInt("key_2","left_type",1,ini_name);
	key_setting[1][13]=GetPrivateProfileInt("key_2","left_code",DIK_NUMPAD4,ini_name);
	key_setting[1][14]=GetPrivateProfileInt("key_2","right_type",1,ini_name);
	key_setting[1][15]=GetPrivateProfileInt("key_2","right_code",DIK_NUMPAD6,ini_name);

	// システムキー
	fast_forwerd[0]=GetPrivateProfileInt("sys_key","fast_type",1,ini_name);
	fast_forwerd[1]=GetPrivateProfileInt("sys_key","fast_code",DIK_TAB,ini_name);
	save_key[0]=GetPrivateProfileInt("sys_key","save_type",1,ini_name);
	save_key[1]=GetPrivateProfileInt("sys_key","save_code",DIK_F5,ini_name);
	load_key[0]=GetPrivateProfileInt("sys_key","load_type",1,ini_name);
	load_key[1]=GetPrivateProfileInt("sys_key","load_code",DIK_F7,ini_name);
	auto_key[0]=GetPrivateProfileInt("sys_key","auto_type",1,ini_name);
	auto_key[1]=GetPrivateProfileInt("sys_key","auto_code",0x1d,ini_name);
	pause_key[0]=GetPrivateProfileInt("sys_key","pause_type",1,ini_name);
	pause_key[1]=GetPrivateProfileInt("sys_key","pause_code",0x9d,ini_name);

	// コロコロカービィ
	koro_use_analog=GetPrivateProfileInt("korokoro","use_analog",0,ini_name)?true:false;
	koro_sensitive=GetPrivateProfileInt("korokoro","sensitivity",100,ini_name);
	koro_key[0]=GetPrivateProfileInt("korokoro","up_type",1,ini_name);
	koro_key[1]=GetPrivateProfileInt("korokoro","up_code",DIK_NUMPAD8,ini_name);
	koro_key[2]=GetPrivateProfileInt("korokoro","down_type",1,ini_name);
	koro_key[3]=GetPrivateProfileInt("korokoro","down_code",DIK_NUMPAD2,ini_name);
	koro_key[4]=GetPrivateProfileInt("korokoro","left_type",1,ini_name);
	koro_key[5]=GetPrivateProfileInt("korokoro","left_code",DIK_NUMPAD4,ini_name);
	koro_key[6]=GetPrivateProfileInt("korokoro","right_type",1,ini_name);
	koro_key[7]=GetPrivateProfileInt("korokoro","right_code",DIK_NUMPAD6,ini_name);

	// その他
	use_ffb=GetPrivateProfileInt("special","use_ffb",0,ini_name)?true:false;
	gb_type=GetPrivateProfileInt("special","gb_type",0,ini_name);
	use_gba=GetPrivateProfileInt("special","use_gba",0,ini_name)?true:false;
	render_pass=GetPrivateProfileInt("special","render_pass",2,ini_name);
	priority_class=GetPrivateProfileInt("special","priority_class",3,ini_name);
	vsync=GetPrivateProfileInt("special","vsync",0,ini_name)?true:false;

	// ウインドウ位置
	win_pos[0]=GetPrivateProfileInt("win_pos","left",100,ini_name);
	win_pos[1]=GetPrivateProfileInt("win_pos","top",100,ini_name);
	win_pos[2]=GetPrivateProfileInt("win_pos","width",(GetSystemMetrics(SM_CXFIXEDFRAME)+1)*2+320,ini_name);
	win_pos[3]=GetPrivateProfileInt("win_pos","height",(GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYMENU)+1)*2+288,ini_name);

	// 速度
	frame_skip=GetPrivateProfileInt("speed","normal_frame_skip",0,ini_name);
	virtual_fps=GetPrivateProfileInt("speed","normal_fps",60,ini_name);
	speed_limit=(GetPrivateProfileInt("speed","normal_limit",1,ini_name))?true:false;
	fast_frame_skip=GetPrivateProfileInt("speed","fast_frame_skip",9,ini_name);
	fast_virtual_fps=GetPrivateProfileInt("speed","fast_fps",9999,ini_name);
	fast_speed_limit=(GetPrivateProfileInt("speed","fast_limit",0,ini_name))?true:false;
	show_fps=(GetPrivateProfileInt("speed","show_fps",0,ini_name))?true:false;

	// カラーフィルタ
	r_def=GetPrivateProfileInt("filter","r_def",0,ini_name);
	g_def=GetPrivateProfileInt("filter","g_def",0,ini_name);
	b_def=GetPrivateProfileInt("filter","b_def",0,ini_name);
	r_div=GetPrivateProfileInt("filter","r_div",256,ini_name);
	g_div=GetPrivateProfileInt("filter","g_div",256,ini_name);
	b_div=GetPrivateProfileInt("filter","b_div",256,ini_name);
	r_r=GetPrivateProfileInt("filter","r_r",256,ini_name);
	r_g=GetPrivateProfileInt("filter","r_g",0,ini_name);
	r_b=GetPrivateProfileInt("filter","r_b",0,ini_name);
	g_r=GetPrivateProfileInt("filter","g_r",0,ini_name);
	g_g=GetPrivateProfileInt("filter","g_g",256,ini_name);
	g_b=GetPrivateProfileInt("filter","g_b",0,ini_name);
	b_r=GetPrivateProfileInt("filter","b_r",0,ini_name);
	b_g=GetPrivateProfileInt("filter","b_g",0,ini_name);
	b_b=GetPrivateProfileInt("filter","b_b",256,ini_name);

	// サウンド
	sound_enable[0]=GetPrivateProfileInt("sound","sq_wav1",1,ini_name);
	sound_enable[1]=GetPrivateProfileInt("sound","sq_wav2",1,ini_name);
	sound_enable[2]=GetPrivateProfileInt("sound","sq_voluntary",1,ini_name);
	sound_enable[3]=GetPrivateProfileInt("sound","sq_noise",1,ini_name);
	sound_enable[4]=GetPrivateProfileInt("sound","master",1,ini_name);
	b_echo=GetPrivateProfileInt("sound","echo",0,ini_name)?true:false;
	b_lowpass=GetPrivateProfileInt("sound","lowpass_filter",1,ini_name)?true:false;

	// IP アドレス
	GetPrivateProfileString("ip_addr","addr_1","",ip_addrs[0],20,ini_name);
	GetPrivateProfileString("ip_addr","addr_2","",ip_addrs[1],20,ini_name);
	GetPrivateProfileString("ip_addr","addr_3","",ip_addrs[2],20,ini_name);
	GetPrivateProfileString("ip_addr","addr_4","",ip_addrs[3],20,ini_name);

	// ディレクトリ
	char tmp_save[256];

	strcpy(tmp_save,home_dir);
	strcat(tmp_save,"\\save");
	GetPrivateProfileString("directory","save_dir",tmp_save,save_dir,256,ini_name);

	if (!SetCurrentDirectory(save_dir))
		CreateDirectory(save_dir,NULL);

	strcpy(tmp_save,home_dir);
	strcat(tmp_save,"\\media");
	GetPrivateProfileString("directory","media_dir",tmp_save,media_dir,256,ini_name);

	if (!SetCurrentDirectory(media_dir))
		CreateDirectory(media_dir,NULL);

	strcpy(dev_dir,home_dir);
	strcat(dev_dir,"\\devices");

	GetPrivateProfileString("directory","last_load","error",cur_dir,256,ini_name);
	if (strcmp(cur_dir,"error")!=0)
		SetCurrentDirectory(cur_dir);
}

setting::~setting()
{
	char tmp[256];
	char ini_name[256];
	strcpy(ini_name,home_dir);
	strcat(ini_name,"\\TGB.ini");

	// ディレクトリ
	GetCurrentDirectory(256,tmp);
	WritePrivateProfileString("directory","last_load",tmp,ini_name);
	WritePrivateProfileString("directory","save_dir",save_dir,ini_name);
	WritePrivateProfileString("directory","media_dir",media_dir,ini_name);

	// キー情報
	sprintf(tmp,"%d",key_setting[0][0]);
	WritePrivateProfileString("key_1","a_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][1]);
	WritePrivateProfileString("key_1","a_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][2]);
	WritePrivateProfileString("key_1","b_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][3]);
	WritePrivateProfileString("key_1","b_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][4]);
	WritePrivateProfileString("key_1","select_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][5]);
	WritePrivateProfileString("key_1","select_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][6]);
	WritePrivateProfileString("key_1","start_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][7]);
	WritePrivateProfileString("key_1","start_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][8]);
	WritePrivateProfileString("key_1","down_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][9]);
	WritePrivateProfileString("key_1","down_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][10]);
	WritePrivateProfileString("key_1","up_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][11]);
	WritePrivateProfileString("key_1","up_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][12]);
	WritePrivateProfileString("key_1","left_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][13]);
	WritePrivateProfileString("key_1","left_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][14]);
	WritePrivateProfileString("key_1","right_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[0][15]);
	WritePrivateProfileString("key_1","right_code",tmp,ini_name);

	sprintf(tmp,"%d",key_setting[1][0]);
	WritePrivateProfileString("key_2","a_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][1]);
	WritePrivateProfileString("key_2","a_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][2]);
	WritePrivateProfileString("key_2","b_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][3]);
	WritePrivateProfileString("key_2","b_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][4]);
	WritePrivateProfileString("key_2","select_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][5]);
	WritePrivateProfileString("key_2","select_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][6]);
	WritePrivateProfileString("key_2","start_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][7]);
	WritePrivateProfileString("key_2","start_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][8]);
	WritePrivateProfileString("key_2","down_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][9]);
	WritePrivateProfileString("key_2","down_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][10]);
	WritePrivateProfileString("key_2","up_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][11]);
	WritePrivateProfileString("key_2","up_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][12]);
	WritePrivateProfileString("key_2","left_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][13]);
	WritePrivateProfileString("key_2","left_code",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][14]);
	WritePrivateProfileString("key_2","right_type",tmp,ini_name);
	sprintf(tmp,"%d",key_setting[1][15]);
	WritePrivateProfileString("key_2","right_code",tmp,ini_name);

	// システムキー
	sprintf(tmp,"%d",fast_forwerd[0]);
	WritePrivateProfileString("sys_key","fast_type",tmp,ini_name);
	sprintf(tmp,"%d",fast_forwerd[1]);
	WritePrivateProfileString("sys_key","fast_code",tmp,ini_name);
	sprintf(tmp,"%d",save_key[0]);
	WritePrivateProfileString("sys_key","save_type",tmp,ini_name);
	sprintf(tmp,"%d",save_key[1]);
	WritePrivateProfileString("sys_key","save_code",tmp,ini_name);
	sprintf(tmp,"%d",load_key[0]);
	WritePrivateProfileString("sys_key","load_type",tmp,ini_name);
	sprintf(tmp,"%d",load_key[1]);
	WritePrivateProfileString("sys_key","load_code",tmp,ini_name);
	sprintf(tmp,"%d",auto_key[0]);
	WritePrivateProfileString("sys_key","auto_type",tmp,ini_name);
	sprintf(tmp,"%d",auto_key[1]);
	WritePrivateProfileString("sys_key","auto_code",tmp,ini_name);
	sprintf(tmp,"%d",pause_key[0]);
	WritePrivateProfileString("sys_key","pause_type",tmp,ini_name);
	sprintf(tmp,"%d",pause_key[1]);
	WritePrivateProfileString("sys_key","pause_code",tmp,ini_name);

	// コロコロカービィ
	sprintf(tmp,"%d",koro_use_analog?1:0);
	WritePrivateProfileString("korokoro","use_analog",tmp,ini_name);
	sprintf(tmp,"%d",koro_sensitive);
	WritePrivateProfileString("korokoro","sensitivity",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[0]);
	WritePrivateProfileString("korokoro","up_type",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[1]);
	WritePrivateProfileString("korokoro","up_code",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[2]);
	WritePrivateProfileString("korokoro","down_type",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[3]);
	WritePrivateProfileString("korokoro","down_code",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[4]);
	WritePrivateProfileString("korokoro","left_type",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[5]);
	WritePrivateProfileString("korokoro","left_code",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[6]);
	WritePrivateProfileString("korokoro","right_type",tmp,ini_name);
	sprintf(tmp,"%d",koro_key[7]);
	WritePrivateProfileString("korokoro","right_code",tmp,ini_name);

	// その他
	sprintf(tmp,"%d",use_ffb?1:0);
	WritePrivateProfileString("special","use_ffb",tmp,ini_name);
	sprintf(tmp,"%d",gb_type);
	WritePrivateProfileString("special","gb_type",tmp,ini_name);
	sprintf(tmp,"%d",use_gba?1:0);
	WritePrivateProfileString("special","use_gba",tmp,ini_name);
	sprintf(tmp,"%d",render_pass);
	WritePrivateProfileString("special","render_pass",tmp,ini_name);
	sprintf(tmp,"%d",priority_class);
	WritePrivateProfileString("special","priority_class",tmp,ini_name);
	sprintf(tmp,"%d",vsync?1:0);
	WritePrivateProfileString("special","vsync",tmp,ini_name);

	// ウインドウ位置
	sprintf(tmp,"%d",win_pos[0]);
	WritePrivateProfileString("win_pos","left",tmp,ini_name);
	sprintf(tmp,"%d",win_pos[1]);
	WritePrivateProfileString("win_pos","top",tmp,ini_name);
	sprintf(tmp,"%d",win_pos[2]);
	WritePrivateProfileString("win_pos","width",tmp,ini_name);
	sprintf(tmp,"%d",win_pos[3]);
	WritePrivateProfileString("win_pos","height",tmp,ini_name);

	// 速度
	sprintf(tmp,"%d",frame_skip);
	WritePrivateProfileString("speed","normal_frame_skip",tmp,ini_name);
	sprintf(tmp,"%d",virtual_fps);
	WritePrivateProfileString("speed","normal_fps",tmp,ini_name);
	sprintf(tmp,"%d",speed_limit?1:0);
	WritePrivateProfileString("speed","normal_limit",tmp,ini_name);
	sprintf(tmp,"%d",fast_frame_skip);
	WritePrivateProfileString("speed","fast_frame_skip",tmp,ini_name);
	sprintf(tmp,"%d",fast_virtual_fps);
	WritePrivateProfileString("speed","fast_fps",tmp,ini_name);
	sprintf(tmp,"%d",fast_speed_limit?1:0);
	WritePrivateProfileString("speed","fast_limit",tmp,ini_name);
	sprintf(tmp,"%d",show_fps?1:0);
	WritePrivateProfileString("speed","show_fps",tmp,ini_name);

	// カラーフィルタ
	sprintf(tmp,"%d",r_def);
	WritePrivateProfileString("filter","r_def",tmp,ini_name);
	sprintf(tmp,"%d",g_def);
	WritePrivateProfileString("filter","g_def",tmp,ini_name);
	sprintf(tmp,"%d",b_def);
	WritePrivateProfileString("filter","b_def",tmp,ini_name);
	sprintf(tmp,"%d",r_div);
	WritePrivateProfileString("filter","r_div",tmp,ini_name);
	sprintf(tmp,"%d",g_div);
	WritePrivateProfileString("filter","g_div",tmp,ini_name);
	sprintf(tmp,"%d",b_div);
	WritePrivateProfileString("filter","b_div",tmp,ini_name);
	sprintf(tmp,"%d",r_r);
	WritePrivateProfileString("filter","r_r",tmp,ini_name);
	sprintf(tmp,"%d",r_g);
	WritePrivateProfileString("filter","r_g",tmp,ini_name);
	sprintf(tmp,"%d",r_b);
	WritePrivateProfileString("filter","r_b",tmp,ini_name);
	sprintf(tmp,"%d",g_r);
	WritePrivateProfileString("filter","g_r",tmp,ini_name);
	sprintf(tmp,"%d",g_g);
	WritePrivateProfileString("filter","g_g",tmp,ini_name);
	sprintf(tmp,"%d",g_b);
	WritePrivateProfileString("filter","g_b",tmp,ini_name);
	sprintf(tmp,"%d",b_r);
	WritePrivateProfileString("filter","b_r",tmp,ini_name);
	sprintf(tmp,"%d",b_g);
	WritePrivateProfileString("filter","b_g",tmp,ini_name);
	sprintf(tmp,"%d",b_b);
	WritePrivateProfileString("filter","b_b",tmp,ini_name);

	// サウンド
	sprintf(tmp,"%d",sound_enable[0]);
	WritePrivateProfileString("sound","sq_wav1",tmp,ini_name);
	sprintf(tmp,"%d",sound_enable[1]);
	WritePrivateProfileString("sound","sq_wav2",tmp,ini_name);
	sprintf(tmp,"%d",sound_enable[2]);
	WritePrivateProfileString("sound","sq_voluntary",tmp,ini_name);
	sprintf(tmp,"%d",sound_enable[3]);
	WritePrivateProfileString("sound","sq_noise",tmp,ini_name);
	sprintf(tmp,"%d",sound_enable[4]);
	WritePrivateProfileString("sound","master",tmp,ini_name);
	sprintf(tmp,"%d",b_echo?1:0);
	WritePrivateProfileString("sound","echo",tmp,ini_name);
	sprintf(tmp,"%d",b_lowpass?1:0);
	WritePrivateProfileString("sound","lowpass_filter",tmp,ini_name);

	// IP アドレス
	WritePrivateProfileString("ip_addr","addr_1",ip_addrs[0],ini_name);
	WritePrivateProfileString("ip_addr","addr_2",ip_addrs[1],ini_name);
	WritePrivateProfileString("ip_addr","addr_3",ip_addrs[2],ini_name);
	WritePrivateProfileString("ip_addr","addr_4",ip_addrs[3],ini_name);
}

void setting::get_key_setting(int *buf,int side)
{
	memcpy(buf,key_setting[side],sizeof(int)*16);
}

void setting::set_key_setting(int *dat,int side)
{
	memcpy(key_setting[side],dat,sizeof(int)*16);
}

void setting::get_save_dir(char *buf)
{
	strcpy(buf,save_dir);
}

void setting::set_save_dir(char *dat)
{
	strcpy(save_dir,dat);
}

void setting::get_media_dir(char *buf)
{
	strcpy(buf,media_dir);
}

void setting::set_media_dir(char *dat)
{
	strcpy(media_dir,dat);
}

void setting::get_dev_dir(char *buf)
{
	strcpy(buf,dev_dir);
}

void setting::get_home_dir(char *buf)
{
	strcpy(buf,home_dir);
}
