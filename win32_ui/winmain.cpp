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

#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <richedit.h>
#include <htmlhelp.h>
#include <ShlObj.h>
#include <mbstring.h>
#include <list>
#include <process.h>
#include <time.h>
#include "../gb_core/gb.h"
#include "../gbr_interface/gbr.h"
#include "dx_renderer.h"
#include "dmy_renderer.h"
#include "setting.h"
#include "resource.h"
#include "network.h"

#define hide

static HINSTANCE hInstance;
static HWND hWnd,hWnd_sub,mes_hwnd,trans_hwnd,chat_hwnd;
static bool sram_transfer_rest=false;
static bool b_running=true;

gb *g_gb[2];
gbr *g_gbr;
dx_renderer *render[2];
#ifndef hide
dx_renderer *dmy_render;
#else
dmy_renderer *dmy_render;
#endif
setting *config;
std::list<char*> mes_list,chat_list;

LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK WndProc2(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
FILE *key_file;
FILE *mov_file;

struct netplay_data{
	int key;
	DWORD time;
};
typedef netplay<netplay_data> tgb_netplay;
tgb_netplay *net=NULL;
int sended=0;

int cur_mode;
enum mode{
	UNLOADED,
	NORMAL_MODE,
	GBR_MODE,
	NETWORK_PREPARING,
	NETWORK_MODE,
};

static const char* WINDOW_TITLE_UNLOADED="TGB Dual [no file loaded]";

#include "dialogs.h"

static void normal_mode();
static void network_preparing();
static void network_mode();

int APIENTRY WinMain(HINSTANCE hInst,HINSTANCE hPrev,LPSTR lpCmdLine,int nCmdShow)
{
	hInstance=hInst;

	if (pre_execute())
		return 0;
	os_check();

	// ほかの初期化に先駆けて設定を読み込む
	config=new setting();
	srand(time(NULL));

	// winsock初期化
	winsock_initializer ws_init;

	// ウインドウの作成
	WNDCLASS wc={CS_HREDRAW|CS_VREDRAW,WndProc,0,0,hInst,LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAIN)),LoadCursor(NULL,IDC_ARROW),
		(HBRUSH)GetStockObject(BLACK_BRUSH),MAKEINTRESOURCE(IDR_MENU),"gb emu \"tgb\""};
	RegisterClass(&wc);

	hWnd=CreateWindow("gb emu \"tgb\"",WINDOW_TITLE_UNLOADED,WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX,
		config->win_pos[0],config->win_pos[1],config->win_pos[2],config->win_pos[3],0L,0L,hInst,0L);

	MSG msg;
	HACCEL hAccel;

	ShowWindow(hWnd,nCmdShow);
	DragAcceptFiles(hWnd,TRUE);
	UpdateWindow(hWnd);

	HMODULE hModule;
	hModule=LoadLibrary("RICHED32.dll");
	hAccel=LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_ACCELERATOR));

	mes_list.push_back("TGB Dual Ver.  Vol.8'''\n");

	// プロセスのプライオリティ
	static DWORD priol[]={REALTIME_PRIORITY_CLASS,HIGH_PRIORITY_CLASS,0x00008000,NORMAL_PRIORITY_CLASS,0x00004000,IDLE_PRIORITY_CLASS};
	SetPriorityClass(GetCurrentProcess(),priol[config->priority_class]);

	render[0]=new dx_renderer(hWnd,hInst);
	render[1]=NULL;
	dmy_render=NULL;
	g_gb[0]=g_gb[1]=NULL;
	render[0]->set_vsync(config->vsync);

	render[0]->set_render_pass(config->render_pass);
	render[0]->show_fps(config->show_fps);

	cur_mode=UNLOADED;

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

	init_devices();

	purse_cmdline(lpCmdLine);

	// メッセージループ
	for(;;){
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
			if (msg.message==WM_QUIT)
				break;
			if(!TranslateAccelerator(msg.hwnd,hAccel,&msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else{
			if (cur_mode==NORMAL_MODE)
				normal_mode();
			if (cur_mode==NETWORK_PREPARING)
				network_preparing();
			else if (cur_mode==NETWORK_MODE)
				network_mode();
			else if (cur_mode==UNLOADED)
				Sleep(10);
		}
	}		

	FreeLibrary(hModule);

	trash_process();

	return msg.wParam;
}

static void normal_mode()
{
	if (GetActiveWindow()) render[0]->enable_check_pad();
	else render[0]->disable_check_pad();

	// とりあえず実行
	for (int line=0;line<154;line++){
		if (g_gb[0])
			g_gb[0]->run();
		if (g_gb[1])
			g_gb[1]->run();
	}
	if (g_gbr)
		g_gbr->run();

	// フレームスキップ周りの処理
	key_dat tmp_key;
	tmp_key.device_type=config->fast_forwerd[0];
	tmp_key.key_code=config->fast_forwerd[1];
	bool fast=render[0]->check_press(&tmp_key);
	int frame_skip=fast?config->fast_frame_skip:config->frame_skip;
	int fps=fast?config->fast_virtual_fps:config->virtual_fps;
	bool limit=fast?config->fast_speed_limit:config->speed_limit;

	if (g_gb[0]) g_gb[0]->set_skip(frame_skip);
	if (render[0]) render[0]->set_mul(frame_skip+1);
	if (limit) elapse_time(fps);
}

static void network_preparing()
{
	if (!net->done_prepare()){
		Sleep(10); // 適当に休みをいれる
		return;
	}

	// SRAMを適用
	pair<char*,int> p=net->get_sram();
	memcpy(g_gb[1]->get_rom()->get_sram(),p.first,p.second);

	ShowWindow(CreateDialog(hInstance,MAKEINTRESOURCE(IDD_CHAT),hWnd,ChatProc),SW_SHOW);

	{
		static char buf[256];
		sprintf(buf,"network latency : %d ms (both-way) \n",net->network_delay());
		mes_list.push_back(buf);
	}

	cur_mode=NETWORK_MODE;
	render[0]->set_timer_state(0);
	render[0]->set_time_fix(true);

	// 接続準備ウインドウを閉じる
	if (trans_hwnd)
		DestroyWindow(trans_hwnd);
}

static void network_mode()
{
	static int cnt=0;
	int send_limit=net->network_delay()/2*60/1000+1;

	// 切られているかどうかの判別
	if (!net->connected()){
		delete net;
		net=NULL;
		// 色々開放
		free_rom(0);
		free_rom(1);
		// チャットウインドウを壊す
		DestroyWindow(chat_hwnd);

		MessageBox(hWnd,"接続が切断されました","TGB Dual NetPlay Notification",MB_OK);
		cur_mode=UNLOADED;
		return;
	}

	// チャットの処理
	while(net->get_message_num()>0){
		string s=net->get_message();
		send_chat_message(s);
	}

	// とりあえず今のところはキーが来次第使用していく
	if (net->get_keydata_num()>0){
		pair<netplay_data,netplay_data> p=net->pop_keydata();

		render[0]->set_pad(p.first.key);
		dmy_render->set_pad(p.second.key);

		render[0]->set_fixed_time((net->is_server()?p.first:p.second).time);
		dmy_render->set_fixed_time((net->is_server()?p.first:p.second).time);

		for (int i=0;i<154;i++){
			g_gb[b_server?0:1]->run();
			g_gb[b_server?1:0]->run();
		}
		sended--;
	}

	while (sended<=send_limit){
		render[0]->update_pad();
		netplay_data nd={GetActiveWindow()==hWnd?render[0]->check_pad():0,time(NULL)};
		net->send_keydata(nd);
		sended++;
	}

	elapse_time(60); // とりあえず固定ということで
}

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
			if (cur_mode!=NORMAL_MODE) break;
			if (g_gb[0]) { g_gb[0]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));g_gb[0]->reset(); }
			if (render[0]) render[0]->reset();
			break;
		case ID_RESET2:
			if (cur_mode!=NORMAL_MODE) break;
			if (g_gb[1]) { g_gb[1]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));g_gb[1]->reset(); }
			if (render[1]) render[1]->reset();
			break;
		case ID_RELEASE_1:
			if (cur_mode!=NORMAL_MODE) break;
			free_rom(0);
			break;
		case ID_RELEASE_2:
			if (cur_mode!=NORMAL_MODE) break;
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
			if (cur_mode!=NORMAL_MODE) break;
			if (g_gb[0]){
				char cur_di[256],sv_dir[256];
				GetCurrentDirectory(256,cur_di);
				config->get_save_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				char name[256],*p;
				strcpy(name,tmp_sram_name[0]);
				p=(char*)_mbsrchr((unsigned char*)name,(unsigned int)'.');
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
			if (cur_mode!=NORMAL_MODE) break;
			if (g_gb[0]){
				char cur_di[256],sv_dir[256];
				char mes[32];
				GetCurrentDirectory(256,cur_di);
				config->get_save_dir(sv_dir);
				SetCurrentDirectory(sv_dir);

				int sav_slot;
				if ((int)lParam==-1)
					sav_slot=((GetKeyState('1')&0xFFF0)?1:((GetKeyState('2')&0xFFF0)?2:((GetKeyState('3')&0xFFF0)?3:((GetKeyState('4')&0xFFF0)?4:((GetKeyState('5')&0xFFF0)?5:(
						(GetKeyState('6')&0xFFF0)?6:((GetKeyState('7')&0xFFF0)?7:((GetKeyState('8')&0xFFF0)?8:((GetKeyState('9')&0xFFF0)?9:0)))))))));
				else
					sav_slot=(int)lParam;

				FILE *file=NULL;
				for (int i=0;i<2;i++){
					char name[256];
					strcpy(name,tmp_sram_name[0]);
					if (i==0) sprintf((char*)_mbsrchr((unsigned char*)name,(unsigned int)'.'),".sv%d",sav_slot);
					else sprintf(strstr(name,"."),".sv%d",sav_slot);
					if (file=fopen(name,"rb")) break;
				}

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
					if (strstr(name,".tmv")==NULL) strcat(name,".tmv");
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (GetOpenFileName(&ofn)==IDOK){
				load_rom(buf,n);
				cur_mode=NORMAL_MODE;
			}
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (MessageBox(hwnd,"OSの応答が非常に悪くなる可能性があります。よろしいですか？","TGB Dual",MB_YESNO)==IDYES){
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
		case ID_VSYNC:
			config->vsync=!config->vsync;
			if (render[0]) render[0]->set_vsync(config->vsync);
			break;
		case ID_MACHINE_GB:
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
			DestroyWindow(hwnd);
			break;
		default:
			if ((LOWORD(wParam)>=ID_ENVIRONMRNT)&&(LOWORD(wParam)<(ID_ENVIRONMRNT+256))&&(g_gb[0])){
				if (dev_loaded)
					trush_device();
				create_device(dll_dat[LOWORD(wParam)-ID_ENVIRONMRNT].file_name);
			}
			else if ((LOWORD(wParam)>=ID_SAVE_DMY)&&(LOWORD(wParam)<(ID_SAVE_DMY+10))&&(g_gb[0])){
				if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
				render[0]->set_save_resurve(LOWORD(wParam)-ID_SAVE_DMY);
			}
			else if ((LOWORD(wParam)>=ID_LOAD_DMY)&&(LOWORD(wParam)<(ID_LOAD_DMY+10))&&(g_gb[0])){
				if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
				render[0]->set_load_resurve(LOWORD(wParam)-ID_LOAD_DMY);
			}
			else if ((LOWORD(wParam)>=ID_TGBHELP)&&(LOWORD(wParam)<(ID_TGBHELP+64))){
				view_help(hwnd,tgb_help[LOWORD(wParam)-ID_TGBHELP]);
			}
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		if (dev_loaded){
			trush_device();
			FreeLibrary(hDevDll);
		}
		if (g_gb[0]) free_rom(0);
		if (g_gb[1]) free_rom(1);

		if (render[0]){
			delete render[0];
			render[0]=NULL;
		}
		if (net){
			delete net;
			net=NULL;
		}

		mes_list.clear();
		chat_list.clear();
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
			mii.fState=config->vsync?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(hMenu,ID_VSYNC,FALSE,&mii);
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
				int j;
				for (j=0;j<2;j++){
					char name[256],*p;
					strcpy(name,tmp_sram_name[0]);
					if (j==0){
						p=(char*)_mbsrchr((unsigned char*)name,(unsigned int)'.');
						if (!p) { j=2;break; }
						sprintf(p,".sv%d",i);
					}
					else if (j==1)
						sprintf(strstr(name,"."),".sv%d",i);

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
						AppendMenu(hMenu,MF_ENABLED,ID_SAVE_DMY+i,tmp);
						break;
					}
				}
				if (j==2){
					sprintf(tmp,"%d : ----/--/-- --:--:--",i);
					AppendMenu(hMenu,MF_ENABLED,ID_SAVE_DMY+i,tmp);
				}
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
				int j;
				for (j=0;j<2;j++){
					char name[256],*p;
					strcpy(name,tmp_sram_name[0]);
					if (j==0){
						p=(char*)_mbsrchr((unsigned char*)name,(unsigned int)'.');
						if (!p) { j=2;break; }
						sprintf(p,".sv%d",i);
					}
					else if (j==1)
						sprintf(strstr(name,"."),".sv%d",i);

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
						break;
					}
				}
				if (j==2){
					sprintf(tmp,"%d : ----/--/-- --:--:--",i);
					ZeroMemory(&mii,sizeof(mii));
					mii.cbSize=sizeof(mii);
					mii.fMask=MIIM_STATE|MIIM_TYPE|MIIM_ID;
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
		if (cur_mode==NETWORK_MODE||cur_mode==NETWORK_PREPARING) break;
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
		cur_mode=NORMAL_MODE;

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
//	case WM_SOCKET:
//		g_sock->handle_message(wParam,lParam);
//		break;
	default:
		return DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	return 0;
}

LRESULT CALLBACK WndProc2(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch( uMsg )
	{
	case WM_CLOSE:
		hWnd_sub=NULL;
		CloseWindow(hwnd);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		free_rom(1);
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
