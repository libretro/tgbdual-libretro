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

#include <list>
#include "../gb_core/gb.h"
///#include "../gbr_interface/gbr.h"
#include "sdl_renderer.h"
#include "dmy_renderer.h"
#include "setting.h"
#include "resource.h"
#include "w32_posix.h"
#include "sock.h"

#include <SDL.h>

#define hide

///static HINSTANCE hInstance;
///static HWND hWnd,hWnd_sub,mes_hwnd,trans_hwnd,chat_hwnd;
static bool sram_transfer_rest=false;
bool b_running=true;

gb *g_gb[2];
///gbr *g_gbr;
sdl_renderer *render[2];
#ifndef hide
dx_renderer *dmy_render;
#else
dmy_renderer *dmy_render;
#endif
setting *config;
sock *g_sock;
std::list<char*> mes_list,chat_list;
bool chat_rest=false;
char chat_send[256];
Uint8* key_state;

bool b_terminal=false;

//LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
//LRESULT CALLBACK WndProc2(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
FILE *key_file;
FILE *mov_file;

bool end;

//#define TRACE_TRANSFER

#include "dialogs.h"

int main(int argc, char* argv[])
{
	char cur_dir[256];
	GetCurrentDirectory(256, cur_dir);

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK);

	SDL_JoystickOpen(0);
	SDL_JoystickOpen(1);

	config=new setting();

	render[0]=new sdl_renderer();
	render[1]=NULL;
	dmy_render=NULL;
	g_sock=new sock();
	g_gb[0]=g_gb[1]=NULL;

	render[0]->set_render_pass(config->render_pass);
	render[0]->show_fps(config->show_fps);

	printf("load key config\n");

	load_key_config(0);

	key_dat tmp_save_load;
	tmp_save_load.device_type=config->save_key[0];
	tmp_save_load.key_code=config->save_key[1];
	render[0]->set_save_key(&tmp_save_load);
	tmp_save_load.device_type=config->load_key[0];
	tmp_save_load.key_code=config->load_key[1];
	render[0]->set_load_key(&tmp_save_load);
	tmp_save_load.device_type=config->auto_key[0];
	tmp_save_load.key_code=config->auto_key[1];
	render[0]->set_auto_key(&tmp_save_load);
	tmp_save_load.device_type=config->pause_key[0];
	tmp_save_load.key_code=config->pause_key[1];
	render[0]->set_pause_key(&tmp_save_load);
	tmp_save_load.device_type=config->full_key[0];
	tmp_save_load.key_code=config->full_key[1];
	render[0]->set_full_key(&tmp_save_load);
	tmp_save_load.device_type=config->reset_key[0];
	tmp_save_load.key_code=config->reset_key[1];
	render[0]->set_reset_key(&tmp_save_load);
	tmp_save_load.device_type=config->quit_key[0];
	tmp_save_load.key_code=config->quit_key[1];
	render[0]->set_quit_key(&tmp_save_load);

	end = false;

	purse_cmdline(argc, argv);
	if (argc == 2) {
		char tmp_dir[256];
		GetCurrentDirectory(256, tmp_dir);
		SetCurrentDirectory(cur_dir);
		printf("load rom %s\n", argv[1]);
		if (load_rom(argv[1],0) != 0) {
			printf("ERROR: invalid rom, usage: %s rom_name\n", argv[0]);
			end = true;
		}
		SetCurrentDirectory(tmp_dir);
	}
	else {
		printf("ERROR: usage: %s rom_name\n", argv[0]);
		end = true;
	}

	int line=0;
	int phase=0;

	printf("start.\n");

	while(!end){
		// 一時的に
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				end = true;
				break;
			}
		}
//		if (key_state[SDLK_ESCAPE]) end = true;
//		if (key_state[SDLK_q]) end = true;

/**
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
			if (msg.message==WM_QUIT)
				break;
			if(!TranslateAccelerator(msg.hwnd,hAccel,&msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else{
*/
/**
			if (sram_transfer_rest){
				sram_transfer();
			}
			else
*/
//		if (!b_terminal){
				if (!b_running){
					if (render[0])
						render[0]->refresh();
					continue;
				}

/** SDL で実現できる、が、こういうのめんどい。
				if (GetActiveWindow())
*/
					render[0]->enable_check_pad();
/**
				else
					render[0]->disable_check_pad();
*/

				for (line=0;line<154;line++){
					if (g_gb[0])
						g_gb[0]->run();
					if (g_gb[1])
						g_gb[1]->run();
				}
/**
				if (g_gbr)
					g_gbr->run();
*/

				key_dat tmp_key;
				tmp_key.device_type=config->fast_forwerd[0];
				tmp_key.key_code=config->fast_forwerd[1];
				if (!render[0]->check_press(&tmp_key)){ // 通常
					if (g_gb[0]) g_gb[0]->set_skip(config->frame_skip);
					if (render[0]) render[0]->set_mul(config->frame_skip+1);
					if (config->speed_limit){
						elapse_wait=(1000<<16)/config->virtual_fps;
						elapse_time();
					}
				}else{ // Fast Forwerd 状態
					if (g_gb[0]) g_gb[0]->set_skip(config->fast_frame_skip);
					if (render[0]) render[0]->set_mul(config->fast_frame_skip+1);
					if (config->fast_speed_limit){
						elapse_wait=(1000<<16)/config->fast_virtual_fps;
						elapse_time();
					}
				}
//			}
#if 0 // b_terminal is true
			else{
				if (g_sock->is_connected()){
					int tmp;
					static int my_pad_state=0,target_state=0,tar_buf=0;

					if (((end_time-start_time)<=1666)||((phase%((end_time-start_time)/1666))==0)){

						BYTE key_buf[256];
						int size;

						if (!(size=g_sock->recv(key_buf)))
							continue;
						else if (size!=4){
//							if (size!=100)
//								SendMessage(chat_hwnd,WM_OUTLOG,NULL,(LPARAM)"error\n");

							char dest[256];
							sprintf(dest,">> %s\n",(char*)(key_buf+4));
							SendMessage(chat_hwnd,WM_OUTLOG,NULL,(LPARAM)dest);
//							continue;
#ifdef TRACE_TRANSFER
							fprintf(key_file,"mes transfer size=%d mes=%X",size,*(int*)(key_buf+4));
#endif
						}

						if (GetActiveWindow()!=chat_hwnd)
							render[0]->update_pad();
						tmp=render[0]->check_pad();

						target_state=*(int*)key_buf;

/*						if (!b_server){
	//						target_state=*(int*)key_buf;
							target_state=tar_buf;
							tar_buf=*(int*)key_buf;
						}
						else{
	//						target_state=*(int*)key_buf;
							target_state=tar_buf;
							tar_buf=*(int*)key_buf;
						}
*/
#ifdef TRACE_TRANSFER
						fprintf(key_file,"%05d: my = %08X tar = %08X\n",phase,my_pad_state,target_state);
#endif

						render[0]->set_pad(my_pad_state);
						dmy_render->set_pad(target_state);

						my_pad_state=tmp;

						BYTE send_dat[256];
						memcpy(send_dat,&tmp,4);

						if (chat_rest){
							memcpy(send_dat+4,chat_send,strlen(chat_send)+1);
							g_sock->send(send_dat,4+strlen(chat_send)+1);
							chat_rest=false;
						}
						else
							g_sock->send(send_dat,4);
					}

					for (line=0;line<154;line++){
						if (b_server){
							if (g_gb[0])
								g_gb[0]->run();
							if (g_gb[1])
								g_gb[1]->run();
						}
						else{
							if (g_gb[1])
								g_gb[1]->run();
							if (g_gb[0])
								g_gb[0]->run();
						}
					}
					elapse_time();
					phase++;
				}
				else{ // 接続切れたとき
					render[0]->set_sound_renderer(NULL);
					delete g_gb[0];
					g_gb[0]=NULL;
					delete g_gb[1];
					g_gb[1]=NULL;
					delete dmy_render;
					dmy_render=NULL;
					b_terminal=false;
					BYTE *buf;
					buf=(BYTE*)calloc(160*144,2);
					render[0]->render_screen(buf,160,144,16);
					free(buf);
					EndDialog(chat_hwnd,0);
					chat_hwnd=NULL;
					chat_list.clear();
				}
			}
#endif
//		}
	}		
	
#ifdef TRACE_TRANSFER
	fclose(key_file);
#endif

	// from WM_DESTROY
	int has_bat[]={0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,1,0,1,1,0, 0,0,0,0,0,0,0,0}; // 0x20以下
	if (g_gb[0]){
		if (has_bat[(g_gb[0]->get_rom()->get_info()->cart_type>0x20)?3:g_gb[0]->get_rom()->get_info()->cart_type])
			save_sram(g_gb[0]->get_rom()->get_sram(),g_gb[0]->get_rom()->get_info()->ram_size,0);
		delete g_gb[0];
		g_gb[0]=NULL;
	}
	if (g_gb[1]){
		if (has_bat[(g_gb[1]->get_rom()->get_info()->cart_type>0x20)?3:g_gb[1]->get_rom()->get_info()->cart_type])
			save_sram(g_gb[1]->get_rom()->get_sram(),g_gb[1]->get_rom()->get_info()->ram_size,1);
		delete g_gb[1];
		g_gb[1]=NULL;
	}
/**
	if (g_gbr){
		FreeLibrary(h_gbr_dll);
		delete g_gbr;
		g_gbr=NULL;
	}
*/
	if (render[0]){
		delete render[0];
		render[0]=NULL;
	}
	if (render[1]){
		delete render[1];
		render[1]=NULL;
	}
	mes_list.clear();
	chat_list.clear();
	delete g_sock;
	delete config;

	return 0;
}

#if 0
LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int has_bat[]={0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,1,0,1,1,0, 0,0,0,0,0,0,0,0}; // 0x20以下
	int n=0;

	switch( uMsg )
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_SETTRACE:
			{
				static bool trace=true;
				g_gb[0]->get_cpu()->set_trace(trace);
				trace=!trace;
			}
			break;
		case ID_RESET1:
			if (b_terminal)
				break;
			if (g_gb[0]) { g_gb[0]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));g_gb[0]->reset(); }
			if (render[0]) render[0]->reset();
			break;
		case ID_RESET2:
			if (b_terminal)
				break;
			if (g_gb[1]) { g_gb[1]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));g_gb[1]->reset(); }
			if (render[1]) render[1]->reset();
			break;
		case ID_RELEASE_1:
			render[0]->set_sound_renderer(NULL);
			if (g_gb[0]){
				if (has_bat[(g_gb[0]->get_rom()->get_info()->cart_type>0x20)?3:g_gb[0]->get_rom()->get_info()->cart_type])
					save_sram(g_gb[0]->get_rom()->get_sram(),g_gb[0]->get_rom()->get_info()->ram_size,0);
				delete g_gb[0];
				g_gb[0]=NULL;
			}
			if (g_gbr){
				FreeLibrary(h_gbr_dll);
				delete g_gbr;
				g_gbr=NULL;
			}
			{
				BYTE *buf;
				buf=(BYTE*)calloc(160*144,2);
				render[0]->render_screen(buf,160,144,16);
				free(buf);
			}
			break;
		case ID_RELEASE_2:
			if (hWnd_sub)
				SendMessage(hWnd_sub,WM_CLOSE,0,0);
			break;
		case ID_SNAPSHOT:
			if (g_gb[0]){
				char sv_dir[256];
				config->get_media_dir(sv_dir);

				char name[256],*p;
				sprintf(name,"%s\\%s",sv_dir,tmp_sram_name[0]);
				if (!(p=strstr(name,".sav")))
					if (!(p=strstr(name,".ram")))
						break;

				int sav_slot=0;

				for(;;){
					sprintf(p,"_%03d.bmp",sav_slot++);
					FILE *file=fopen(name,"rb");
					if (file)
						fclose(file);
					else
						break;
				};

				render[0]->graphics_record(name);
			}
			break;
		case ID_SOUNDRECORD:
			if (g_gb[0]){
				static int x=0;
				x++;

				if (!(x&1)){
					render[0]->sound_record(NULL);
					break;
				}

				char cur_di[256],sv_dir[256];
				GetCurrentDirectory(256,cur_di);
				config->get_media_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				if (!(p=strstr(name,".sav")))
					if (!(p=strstr(name,".ram")))
						break;

				int sav_slot=0;

				for(;;){
					sprintf(p,"_%03d.wav",sav_slot++);
					FILE *file=fopen(name,"rb");
					if (file)
						fclose(file);
					else
						break;
				};

				render[0]->sound_record(name);

				SetCurrentDirectory(cur_di);
			}
			else if (g_gbr){
				static int x=0;
				x++;

				if (!(x&1)){
					render[0]->sound_record(NULL);
					break;
				}

				char cur_di[256],sv_dir[256];
				GetCurrentDirectory(256,cur_di);
				config->get_media_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				if (!(p=strstr(name,".gbr")))
					break;

				int sav_slot=0;

				for(;;){
					sprintf(p,"_%03d.wav",sav_slot++);
					FILE *file=fopen(name,"rb");
					if (file)
						fclose(file);
					else
						break;
				};

				render[0]->sound_record(name);

				SetCurrentDirectory(cur_di);
			}
			break;
		case ID_SAVE_STATE:
			if (g_gb[0]){
				char cur_di[256],sv_dir[256];
				GetCurrentDirectory(256,cur_di);
				config->get_save_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				if (!(p=strstr(name,".sav")))
					if (!(p=strstr(name,".ram")))
						break;
				int sav_slot;
				
				if ((int)lParam==-1)
					sav_slot=((GetKeyState('1')&0xFFF0)?1:((GetKeyState('2')&0xFFF0)?2:((GetKeyState('3')&0xFFF0)?3:((GetKeyState('4')&0xFFF0)?4:((GetKeyState('5')&0xFFF0)?5:(
						(GetKeyState('6')&0xFFF0)?6:((GetKeyState('7')&0xFFF0)?7:((GetKeyState('8')&0xFFF0)?8:((GetKeyState('9')&0xFFF0)?9:0)))))))));
				else
					sav_slot=(int)lParam;

				sprintf(p,".sv%d",sav_slot);

				FILE *file=fopen(name,"wb");
				g_gb[0]->save_state(file);
				int tmp=render[0]->get_timer_state();
				fseek(file,-100,SEEK_CUR);
				fwrite(&tmp,4,1,file);
				fclose(file);

				char mes[32];
				sprintf(mes,"save state at %d        ",sav_slot);
				render[0]->show_message(mes);

				SetCurrentDirectory(cur_di);
			}
			break;
		case ID_RESTORE_STATE:
			if (g_gb[0]){
				char cur_di[256],sv_dir[256];
				char mes[32];
				GetCurrentDirectory(256,cur_di);
				config->get_save_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				if (!(p=strstr(name,".sav")))
					if (!(p=strstr(name,".ram")))
						break;

				int sav_slot;
				if ((int)lParam==-1)
					sav_slot=((GetKeyState('1')&0xFFF0)?1:((GetKeyState('2')&0xFFF0)?2:((GetKeyState('3')&0xFFF0)?3:((GetKeyState('4')&0xFFF0)?4:((GetKeyState('5')&0xFFF0)?5:(
						(GetKeyState('6')&0xFFF0)?6:((GetKeyState('7')&0xFFF0)?7:((GetKeyState('8')&0xFFF0)?8:((GetKeyState('9')&0xFFF0)?9:0)))))))));
				else
					sav_slot=(int)lParam;
				sprintf(p,".sv%d",sav_slot);

				FILE *file=fopen(name,"rb");

				if (file){
					g_gb[0]->restore_state(file);
					int tmp;
					fseek(file,-100,SEEK_CUR);
					fread(&tmp,4,1,file);
					render[0]->set_timer_state(tmp);
					fclose(file);
					sprintf(mes,"restore state at %d          ",sav_slot);
					render[0]->show_message(mes);
				}
				else{
					sprintf(mes,"can't open state at %d         ",sav_slot);
					render[0]->show_message(mes);
				}

				SetCurrentDirectory(cur_di);
			}
			break;
		case ID_MOVIE_START:
			if (g_gb[0]&&!mov_file){
				char cur_di[256],sv_dir[256];
				GetCurrentDirectory(256,cur_di);
				config->get_media_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256];

				OPENFILENAME ofn;
				ZeroMemory(&ofn,sizeof(ofn));
				ofn.hInstance=hInstance;
				ofn.hwndOwner=hWnd;
				ofn.lStructSize=sizeof(ofn);
				ofn.lpstrDefExt="tmv";
				ofn.lpstrFilter="TGB movie file\0*.tmv\0All Files (*.*)\0*.*\0\0";
				ofn.nMaxFile=256;
				ofn.nMaxFileTitle=256;
				ofn.lpstrFileTitle=name;
				ofn.lpstrTitle="GB Movie Save";
				ofn.lpstrInitialDir=sv_dir;
				if (GetSaveFileName(&ofn)==IDOK){
					mov_file=fopen(name,"wb");
					g_gb[0]->save_state(mov_file);
					int tmp=render[0]->get_timer_state();
					fseek(mov_file,-100,SEEK_CUR);
					fwrite(&tmp,4,1,mov_file);
					fseek(mov_file,96,SEEK_CUR);
					int t_s[16];
					g_gb[0]->get_cpu()->save_state_ex(t_s);
					fwrite(t_s,4,16,mov_file);

					render[0]->movie_record_start(mov_file);
					char mes[32];
					sprintf(mes,"movie recording start...        ");
					render[0]->show_message(mes);
				}

				SetCurrentDirectory(cur_di);
			}
			break;
		case ID_MOVIE_END:
			if (g_gb[0]&&mov_file){
				render[0]->movie_record_stop();
				fclose(mov_file);
				mov_file=NULL;
				char mes[32];
				sprintf(mes,"movie recording stop ...        ");
				render[0]->show_message(mes);
			}
			break;
		case ID_MOVIE_PLAY:
			if (g_gb[0]&&!mov_file){
				char cur_di[256],sv_dir[256];
				GetCurrentDirectory(256,cur_di);
				config->get_media_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256];

				OPENFILENAME ofn;
				ZeroMemory(&ofn,sizeof(ofn));
				ofn.hInstance=hInstance;
				ofn.hwndOwner=hWnd;
				ofn.lStructSize=sizeof(ofn);
				ofn.lpstrDefExt="tmv";
				ofn.lpstrFilter="TGB movie file\0*.tmv\0All Files (*.*)\0*.*\0\0";
				ofn.nMaxFile=256;
				ofn.nMaxFileTitle=256;
				ofn.lpstrFileTitle=name;
				ofn.lpstrTitle="GB Movie Play";
				ofn.lpstrInitialDir=sv_dir;
				if (GetOpenFileName(&ofn)==IDOK){
					mov_file=fopen(name,"rb");
					g_gb[0]->restore_state(mov_file);
					int tmp;
					fseek(mov_file,-100,SEEK_CUR);
					fread(&tmp,4,1,mov_file);
					fseek(mov_file,96,SEEK_CUR);
					render[0]->set_timer_state(tmp);
					int t_s[16];
					fread(t_s,4,16,mov_file);
					g_gb[0]->get_cpu()->restore_state_ex(t_s);

					vector<mov_key> tmp_list;
					for(;;){
						mov_key t_k;
						int i;
						fread(&i,4,1,mov_file);
						t_k.frame=i;
						fread(&i,4,1,mov_file);
						t_k.key_code=i;
						tmp_list.push_back(t_k);
						if (i==0xffffffff)
							break;
					}

					fclose(mov_file);

					render[0]->movie_play_start(&tmp_list);
					char mes[32];
					sprintf(mes,"movie playing start...        ");
					render[0]->show_message(mes);
					mov_file=NULL;
				}

				SetCurrentDirectory(cur_di);
			}
			break;
		case ID_MOVIE_PLAY_STOP:
			if (g_gb[0]){
				render[0]->movie_play_stop();
				char mes[32];
				sprintf(mes,"movie playing stop...        ");
				render[0]->show_message(mes);
			}
			break;
		case ID_LOADROM2:
			if (!hWnd_sub){
				RECT rect;
				GetWindowRect(hWnd,&rect);
				WNDCLASS wc={CS_HREDRAW|CS_VREDRAW,WndProc2,0,0,hInstance,NULL,LoadCursor(hInstance,IDC_ARROW),
					(HBRUSH)GetStockObject(BLACK_BRUSH),NULL,"gb emu \"tgb\" sub win"};
				RegisterClass(&wc);
				hWnd_sub=CreateWindow("gb emu \"tgb\" sub win","2nd",WS_DLGFRAME,rect.right,rect.top+GetSystemMetrics(SM_CYMENU)+1,
					(GetSystemMetrics(SM_CXFIXEDFRAME))*2+320,(GetSystemMetrics(SM_CYFIXEDFRAME))*2+(GetSystemMetrics(SM_CYMENU))+288,0L,0L,hInstance,0L);
				ShowWindow(hWnd_sub,SW_SHOW);
			}
			n++;
		case ID_LOADROM:
			char buf[256],dir[256];
			if (render[0])
				render[0]->pause_sound();
			GetCurrentDirectory(256,dir);
			OPENFILENAME ofn;
			ZeroMemory(&ofn,sizeof(ofn));
			ofn.hInstance=hInstance;
			ofn.hwndOwner=hWnd;
			ofn.lStructSize=sizeof(ofn);
			ofn.lpstrDefExt="gb";
			ofn.lpstrFilter="Game Boy Rom Image (include archive file) (*.gb;*.gbc;*.cab;*.zip;*.rar;*.lzh;*.gbr)\0*.gb;*.gbc;*.cab;*.rar;*.zip;*.lzh;*.gbr\0All Files (*.*)\0*.*\0\0";
			ofn.nMaxFile=256;
			ofn.nMaxFileTitle=256;
			ofn.lpstrFileTitle=buf;
			ofn.lpstrTitle="GB Rom Load";
			ofn.lpstrInitialDir=dir;
			if (GetOpenFileName(&ofn)==IDOK)
				load_rom(buf,n);
			else if (n){
				SendMessage(hWnd_sub,WM_CLOSE,0,0);
				CloseWindow(hWnd_sub);
				if (g_gb[1]){
					save_sram(g_gb[1]->get_rom()->get_sram(),g_gb[1]->get_rom()->get_info()->ram_size,1);
					delete g_gb[1];
					g_gb[1]=NULL;
				}
				if (render[1]){
					delete render[1];
					render[1]=NULL;
				}
			}
			if (render[0])
				render[0]->resume_sound();
			break;
		case ID_PAUSE:
			if (g_gb[0]){
				b_running=!b_running;
				if (render[0])
					render[0]->resume_sound();
				if (!b_running)
					if (render[0])
						render[0]->pause_sound();
			}
			break;
		case ID_CONNECT:
			if ((!g_gb[0])&&(!g_gb[1])){
				trans_hwnd=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_CONNECT),hwnd,ConnectProc);
				ShowWindow(trans_hwnd,SW_SHOW);
			}
			break;
		case ID_SHOWLOG:
			if (!mes_hwnd)
				mes_hwnd=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_LOG),hwnd,LogProc);
			ShowWindow(mes_hwnd,SW_SHOW);
			break;
		case ID_KEY:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_KEY),hwnd,KeyProc),SW_SHOW);
			break;
		case ID_SOUND:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_SOUND),hwnd,SoundProc),SW_SHOW);
			break;
		case ID_SPEED:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_SPEED),hwnd,SpeedProc),SW_SHOW);
			break;
		case ID_FILTER:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_FILTER),hwnd,FilterProc),SW_SHOW);
			break;
		case ID_KOROKORO:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_KOROKORO),hwnd,KorokoroProc),SW_SHOW);
			break;
		case ID_REALTIMECLOCK:
			if (g_gb[0]&&((g_gb[0]->get_rom()->get_info()->cart_type==0x0f)||(g_gb[0]->get_rom()->get_info()->cart_type==0x10)))
				ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_CLOCK),hwnd,ClockProc),SW_SHOW);
			break;
		case ID_DIRECTORY:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIRECTORY),hwnd,DirectoryProc),SW_SHOW);
			break;
		case ID_PAR:
			if (g_gb[0])
				ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_PAR),hwnd,ParProc),SW_SHOW);
			break;
		case ID_CHEAT:
			if (g_gb[0])
				ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_CHEAT),hwnd,CheatProc),SW_SHOW);
			break;
		case ID_MEM_DUMP:
			if (g_gb[0])
				ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MEM_DUMP),hwnd,MemProc),SW_SHOW);
			break;
		case ID_NOMEM_DUMP:
			if (g_gb[0])
				ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MEM_DUMP),hwnd,NoMemProc),SW_SHOW);
			break;
		case ID_VERSION:
			ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_VERSION),hwnd,VerProc),SW_SHOW);
			break;
		case ID_FULLSCREEN:
			render[0]->pause_sound();
			render[0]->swith_screen_mode();
			render[0]->resume_sound();
			if (g_gb[0])
				g_gb[0]->refresh_pal();
			break;
		case ID_X1:
			SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,(GetSystemMetrics(SM_CXFIXEDFRAME)+1)*2+160,(GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYMENU))*2+GetSystemMetrics(SM_CYMENU)+1+144,SWP_NOMOVE|SWP_SHOWWINDOW);
			break;
		case ID_X2:
			SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,(GetSystemMetrics(SM_CXFIXEDFRAME)+1)*2+160*2,(GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYMENU)+1)*2+144*2,SWP_NOMOVE|SWP_SHOWWINDOW);
			break;
		case ID_X3:
			SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,(GetSystemMetrics(SM_CXFIXEDFRAME)+1)*2+160*3,(GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYMENU)+1)*2+144*3,SWP_NOMOVE|SWP_SHOWWINDOW);
			break;
		case ID_X4:
			SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,(GetSystemMetrics(SM_CXFIXEDFRAME)+1)*2+160*4,(GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYMENU)+1)*2+144*4,SWP_NOMOVE|SWP_SHOWWINDOW);
			break;
		case ID_SURFACETYPE_1:
			config->render_pass=0;
			if (render[0]) render[0]->set_render_pass(0);
			if (render[1]) render[1]->set_render_pass(0);
			break;
		case ID_SURFACETYPE_2:
			config->render_pass=1;
			if (render[0]) render[0]->set_render_pass(1);
			if (render[1]) render[1]->set_render_pass(1);
			break;
		case ID_SURFACETYPE_3:
			config->render_pass=2;
			if (render[0]) render[0]->set_render_pass(2);
			if (render[1]) render[1]->set_render_pass(2);
			break;
		case ID_PROCESS_REALTIME:
			if (MessageBox(hwnd,"本当によろしいですか ? 苦情は受け付けませんよ ?","TGB Dual",MB_YESNO)==IDYES){
				config->priority_class=0;
				SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
			}
			break;
		case ID_PROCESS_HIGH:
			config->priority_class=1;
			SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
			break;
		case ID_PROCESS_ABOVE_NORMAL:
			config->priority_class=2;
			SetPriorityClass(GetCurrentProcess(),0x00008000/*ABOVE_NORMAL_PRIORITY_CLASS*/);
			break;
		case ID_PROCESS_NORMAL:
			config->priority_class=3;
			SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
			break;
		case ID_PROCESS_BELOW_NORMAL:
			config->priority_class=4;
			SetPriorityClass(GetCurrentProcess(),0x00004000/*BELOW_NORMAL_PRIORITY_CLASS*/);
			break;
		case ID_PROCESS_IDLE:
			config->priority_class=5;
			SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS);
			break;
		case ID_BG:
			if (g_gb[0])
				g_gb[0]->get_lcd()->set_enable(0,!g_gb[0]->get_lcd()->get_enable(0));
			break;
		case ID_WINDOW:
			if (g_gb[0])
				g_gb[0]->get_lcd()->set_enable(1,!g_gb[0]->get_lcd()->get_enable(1));
			break;
		case ID_SPRITE:
			if (g_gb[0])
				g_gb[0]->get_lcd()->set_enable(2,!g_gb[0]->get_lcd()->get_enable(2));
			break;
		case ID_MACHINE_GB:
			config->gb_type=1;

			if (g_gb[0]){
				g_gb[0]->get_rom()->get_rom()[0x143]&=0x7f;
				g_gb[0]->set_use_gba(false);
				g_gb[0]->reset();
			}
			if (g_gb[1]){
				g_gb[1]->get_rom()->get_rom()[0x143]&=0x7f;
				g_gb[0]->set_use_gba(false);
				g_gb[1]->reset();
			}
			break;
		case ID_MACHINE_GBC:
			config->gb_type=3;
			if (g_gb[0]){
				g_gb[0]->get_rom()->get_rom()[0x143]|=0x80;
				g_gb[0]->set_use_gba(false);
				g_gb[0]->reset();
			}
			if (g_gb[1]){
				g_gb[1]->get_rom()->get_rom()[0x143]|=0x80;
				g_gb[0]->set_use_gba(false);
				g_gb[1]->reset();
			}
			break;
		case ID_MACHINE_GBA:
			config->gb_type=4;
			if (g_gb[0]){
				g_gb[0]->get_rom()->get_rom()[0x143]|=0x80;
				g_gb[0]->set_use_gba(true);
				g_gb[0]->reset();
			}
			if (g_gb[1]){
				g_gb[1]->get_rom()->get_rom()[0x143]|=0x80;
				g_gb[0]->set_use_gba(true);
				g_gb[1]->reset();
			}
			break;
		case ID_DEFAULT_GBA:
			config->use_gba=!config->use_gba;
			if (g_gb[0]){
				g_gb[0]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));
				g_gb[0]->reset();
			}
			if (g_gb[1]){
				g_gb[1]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));
				g_gb[1]->reset();
			}
			break;
		case ID_MACHINE_AUTO:
			config->gb_type=0;
			if (g_gb[0]){
				g_gb[0]->get_rom()->get_rom()[0x143]&=0x7f;
				g_gb[0]->get_rom()->get_rom()[0x143]|=org_gbtype[0]&0x80;
				g_gb[0]->set_use_gba(config->use_gba);
				g_gb[0]->reset();
			}
			if (g_gb[1]){
				g_gb[1]->get_rom()->get_rom()[0x143]&=0x7f;
				g_gb[1]->get_rom()->get_rom()[0x143]|=org_gbtype[1]&0x80;
				g_gb[1]->set_use_gba(config->use_gba);
				g_gb[1]->reset();
			}
			break;
		case ID_EXIT:
			CloseWindow(hwnd);
			DestroyWindow(hwnd);
			break;
		default:
			if ((LOWORD(wParam)>=ID_ENVIRONMRNT)&&(LOWORD(wParam)<(ID_ENVIRONMRNT+256))&&(g_gb[0])){
				if (dev_loaded)
					trush_device();
				create_device(dll_dat[LOWORD(wParam)-ID_ENVIRONMRNT].file_name);
			}
			else if ((LOWORD(wParam)>=ID_SAVE_DMY)&&(LOWORD(wParam)<(ID_SAVE_DMY+10))&&(g_gb[0])){
				render[0]->set_save_resurve(LOWORD(wParam)-ID_SAVE_DMY);
			}
			else if ((LOWORD(wParam)>=ID_LOAD_DMY)&&(LOWORD(wParam)<(ID_LOAD_DMY+10))&&(g_gb[0])){
				render[0]->set_load_resurve(LOWORD(wParam)-ID_LOAD_DMY);
			}
			else if ((LOWORD(wParam)>=ID_TGBHELP)&&(LOWORD(wParam)<(ID_TGBHELP+64))){
				view_help(hwnd,tgb_help[LOWORD(wParam)-ID_TGBHELP]);
			}
			break;
		}
		break;
	case WM_CLOSE:
		if (/*hDevDll*/dev_loaded){
			trush_device();
			FreeLibrary(hDevDll);
		}
		CloseWindow(hwnd);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		if (g_gb[0]){
			if (has_bat[(g_gb[0]->get_rom()->get_info()->cart_type>0x20)?3:g_gb[0]->get_rom()->get_info()->cart_type])
				save_sram(g_gb[0]->get_rom()->get_sram(),g_gb[0]->get_rom()->get_info()->ram_size,0);
			delete g_gb[0];
			g_gb[0]=NULL;
		}
		if (g_gb[1]){
			if (has_bat[(g_gb[1]->get_rom()->get_info()->cart_type>0x20)?3:g_gb[1]->get_rom()->get_info()->cart_type])
				save_sram(g_gb[1]->get_rom()->get_sram(),g_gb[1]->get_rom()->get_info()->ram_size,1);
			delete g_gb[1];
			g_gb[1]=NULL;
		}
		if (g_gbr){
			FreeLibrary(h_gbr_dll);
			delete g_gbr;
			g_gbr=NULL;
		}
		if (render[0]){
			delete render[0];
			render[0]=NULL;
		}
		if (render[1]){
			delete render[1];
			render[1]=NULL;
		}
		mes_list.clear();
		chat_list.clear();
		delete g_sock;
		delete config;
		PostQuitMessage(0);
		break;
	case WM_MOVE:
	case WM_SIZE:
		if ((!((uMsg==WM_SIZE)&&(wParam!=0)))&&render[0]&&render[0]->get_screen_mode()){
			RECT rect;
			GetWindowRect(hWnd,&rect);
			if ((rect.left>0)&&(rect.left<2000)){
				config->win_pos[0]=rect.left;
				config->win_pos[1]=rect.top;
				config->win_pos[2]=rect.right-rect.left;
				config->win_pos[3]=rect.bottom-rect.top;
			}
		}

		if (render[0])
			render[0]->on_move();
		break;
	case WM_ENTERMENULOOP:
		if (render[0]){
			render[0]->draw_menu(1);
			render[0]->pause_sound();
		}
		break;
	case WM_EXITMENULOOP:
		if (render[0]){
			render[0]->draw_menu(0);
			render[0]->resume_sound();
		}
		break;
	case WM_INITMENUPOPUP:
		HMENU hMenu;
		hMenu=GetMenu(hWnd);
		if ((HMENU)wParam==search_menu(hMenu,ID_BG)/*GetSubMenu(GetSubMenu(hMenu,1),3)*/){
			MENUITEMINFO mii;
			memset(&mii,0,sizeof(mii));
			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_STATE;
			mii.fState=(g_gb[0]&&g_gb[0]->get_lcd()->get_enable(0))?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_BG,FALSE,&mii);
			mii.fState=(g_gb[0]&&g_gb[0]->get_lcd()->get_enable(1))?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_WINDOW,FALSE,&mii);
			mii.fState=(g_gb[0]&&g_gb[0]->get_lcd()->get_enable(2))?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_SPRITE,FALSE,&mii);
		}
		else if ((HMENU)wParam==search_menu(hMenu,ID_TGBHELP)){
			hMenu=search_menu(hMenu,ID_TGBHELP);
			while(DeleteMenu(hMenu,0,MF_BYPOSITION));
			construct_help_menu(hMenu);
		}
		else if ((HMENU)wParam==search_menu(hMenu,ID_MACHINE_GB)/*GetSubMenu(GetSubMenu(hMenu,1),7)*/){
			MENUITEMINFO mii;
			memset(&mii,0,sizeof(mii));
			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_STATE;
			mii.fState=(config->gb_type==1)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_MACHINE_GB,FALSE,&mii);
			mii.fState=(config->gb_type==3)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_MACHINE_GBC,FALSE,&mii);
			mii.fState=(config->gb_type==4)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_MACHINE_GBA,FALSE,&mii);
			mii.fState=(config->gb_type==0)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_MACHINE_AUTO,FALSE,&mii);
			mii.fState=(config->use_gba)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_DEFAULT_GBA,FALSE,&mii);
		}
		else if ((HMENU)wParam==search_menu(hMenu,ID_SURFACETYPE_1)/*GetSubMenu(GetSubMenu(hMenu,1),8)*/){
			MENUITEMINFO mii;
			memset(&mii,0,sizeof(mii));
			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_STATE;
			mii.fState=(config->render_pass==0)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_SURFACETYPE_1,FALSE,&mii);
			mii.fState=(config->render_pass==1)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_SURFACETYPE_2,FALSE,&mii);
			mii.fState=(config->render_pass==2)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_SURFACETYPE_3,FALSE,&mii);
		}
		else if ((HMENU)wParam==search_menu(hMenu,ID_PROCESS_REALTIME)){
			MENUITEMINFO mii;
			memset(&mii,0,sizeof(mii));
			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_STATE;
//			mii.fState=MFS_DISABLED;
			mii.fState=(config->priority_class==0)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_PROCESS_REALTIME,FALSE,&mii);
			mii.fState=(config->priority_class==1)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_PROCESS_HIGH,FALSE,&mii);
			mii.fState=(config->priority_class==2)?MFS_CHECKED:MFS_UNCHECKED;
			if (!sys_win2000) mii.fState|=MFS_DISABLED;
			SetMenuItemInfo(hMenu,ID_PROCESS_ABOVE_NORMAL,FALSE,&mii);
			mii.fState=(config->priority_class==3)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_PROCESS_NORMAL,FALSE,&mii);
			mii.fState=(config->priority_class==4)?MFS_CHECKED:MFS_UNCHECKED;
			if (!sys_win2000) mii.fState|=MFS_DISABLED;
			SetMenuItemInfo(hMenu,ID_PROCESS_BELOW_NORMAL,FALSE,&mii);
			mii.fState=(config->priority_class==5)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_PROCESS_IDLE,FALSE,&mii);
		}
		else if ((HMENU)wParam==search_menu(hMenu,ID_SAVE_DMY)/*GetSubMenu(GetSubMenu(hMenu,0),2)*/){ // セーブのほう
			hMenu=search_menu(hMenu,ID_SAVE_DMY);

			char cur_di[256],sv_dir[256],tmp[32];
			HANDLE hFile;
			GetCurrentDirectory(256,cur_di);
			config->get_save_dir(sv_dir);
			SetCurrentDirectory(sv_dir);

			while(DeleteMenu(hMenu,0,MF_BYPOSITION));

			for (int i=0;i<10;i++){
				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				if (!(p=strstr(name,".sav"))){
					if (!(p=strstr(name,".ram"))){
						strcpy(name,"invalid");
					}
				}
				else
					sprintf(p,".sv%d",i);
				hFile=CreateFile(name,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
				if (hFile!=INVALID_HANDLE_VALUE){
					BY_HANDLE_FILE_INFORMATION bhfi;
					FILETIME lft;
					SYSTEMTIME st;
					GetFileInformationByHandle(hFile,&bhfi);
					FileTimeToLocalFileTime(&bhfi.ftLastWriteTime,&lft);
					FileTimeToSystemTime(&lft,&st);
					sprintf(tmp,"%d : %04d/%02d/%02d %02d:%02d:%02d",i,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					CloseHandle(hFile);
				}
				else{
					sprintf(tmp,"%d : ----/--/-- --:--:--",i);
				}
				AppendMenu(hMenu,MF_ENABLED,ID_SAVE_DMY+i,tmp);
			}
			SetCurrentDirectory(cur_di);
		}
		else if ((HMENU)wParam==search_menu(hMenu,ID_LOAD_DMY)/*GetSubMenu(GetSubMenu(hMenu,0),3)*/){ // ロードのほう
			hMenu=search_menu(hMenu,ID_LOAD_DMY);

			char cur_di[256],sv_dir[256],tmp[32];
			HANDLE hFile;
			MENUITEMINFO mii;
			GetCurrentDirectory(256,cur_di);
			config->get_save_dir(sv_dir);
			SetCurrentDirectory(sv_dir);

			while(DeleteMenu(hMenu,0,MF_BYPOSITION));

			for (int i=0;i<10;i++){
				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				if (!(p=strstr(name,".sav"))){
					if (!(p=strstr(name,".ram"))){
						strcpy(name,"invalid");
					}
				}
				else
					sprintf(p,".sv%d",i);
				hFile=CreateFile(name,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
				if (hFile!=INVALID_HANDLE_VALUE){
					BY_HANDLE_FILE_INFORMATION bhfi;
					FILETIME lft;
					SYSTEMTIME st;
					GetFileInformationByHandle(hFile,&bhfi);
					FileTimeToLocalFileTime(&bhfi.ftLastWriteTime,&lft);
					FileTimeToSystemTime(&lft,&st);
					sprintf(tmp,"%d : %04d/%02d/%02d %02d:%02d:%02d",i,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					CloseHandle(hFile);
					AppendMenu(hMenu,MF_ENABLED,ID_LOAD_DMY+i,tmp);
				}
				else{
					sprintf(tmp,"%d : ----/--/-- --:--:--",i);
					ZeroMemory(&mii,sizeof(mii));
					mii.cbSize=sizeof(mii);
					mii.fMask=MIIM_STATE|MIIM_TYPE;
					mii.fState=MFS_GRAYED;
					mii.fType=MFT_STRING;
					mii.wID=ID_LOAD_DMY+i;
					mii.cch=strlen(tmp);
					mii.dwTypeData=tmp;
					InsertMenuItem(hMenu,i,TRUE,&mii);
				}
			}
			SetCurrentDirectory(cur_di);
		}
		break;
	case WM_DROPFILES:
		char bufbuf[256],fn[256];
		char *p,*bef,*pp;
		p=bufbuf;
		DragQueryFile((HDROP)wParam,0,bufbuf,256);
		while((p=(char*)_mbschr((BYTE*)p+1,'\\'))!=NULL) bef=p;
		p=bef;
		strcpy(fn,p+1);
		*p='\0';
		SetCurrentDirectory(bufbuf);

		pp=strstr(fn,".");
		load_rom(fn,0);

		DragFinish((HDROP)wParam);
		break;
	case WM_OUTLOG:
		if (mes_hwnd)
			SendMessage(mes_hwnd,WM_OUTLOG,0,lParam);
		else{
			char *p;
			p=new char[256];
			strcpy(p,(char*)lParam);
			mes_list.push_back(p);
		}
		break;
	case WM_SOCKET:
		g_sock->handle_message(wParam,lParam);
		break;
	default:
		return DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	return 0;
}

LRESULT CALLBACK WndProc2(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int has_bat[]={0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,1,0,1,1,0, 0,0,0,0,0,0,0,0}; // 0x20以下

	switch( uMsg )
	{
	case WM_CLOSE:
		hWnd_sub=NULL;
		CloseWindow(hwnd);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		if (g_gb[1]){
			if (has_bat[(g_gb[1]->get_rom()->get_info()->cart_type>0x20)?3:g_gb[1]->get_rom()->get_info()->cart_type])
				save_sram(g_gb[1]->get_rom()->get_sram(),g_gb[1]->get_rom()->get_info()->ram_size,1);
			delete g_gb[1];
			g_gb[1]=NULL;
			if (g_gb[0])
				g_gb[0]->set_target(NULL);
		}
		if (render[1]){
			delete render[1];
			render[1]=NULL;
		}
		hWnd_sub=NULL;
		break;
	case WM_MOVE:
	case WM_SIZE:
		if (render[1])
			render[1]->on_move();
		break;
	case WM_OUTLOG:
		if (mes_hwnd)
			SendMessage(mes_hwnd,WM_OUTLOG,0,lParam);
		else{
			char *p;
			p=new char[256];
			strcpy(p,(char*)lParam);
			mes_list.push_back(p);
		}
		break;
	default:
		return DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	return 0;
}
#endif


