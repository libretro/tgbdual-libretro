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

//------------------------------------------------------
// interface renderer の DirectXを用いた実装

#include "dx_renderer.h"
#include "resource.h"
#include <stdio.h>
#include <time.h>

dx_renderer::dx_renderer(HWND hwnd,HINSTANCE hInst)
{
	m_hwnd=hwnd;
	m_hinst=hInst;

	b_window=true;
	vsync=false;
	render_pass_type=2;

	init_dd();
	init_ds();
	init_di();

	cur_time=0;
	now_sensor_x=now_sensor_y=2047;
	b_pad_update=true;
	b_time_fix=false;
	fixed_time=0;

	save_resurve=-1;
	load_resurve=-1;

	b_sound=b_graphics=false;
	b_bibrating=false;
	wav=NULL;

	key_list.clear();
	movie_playing=movie_recording=false;
	movie_start=mov_cur_pos=0;
	movie_file=NULL;

	output_log("DirectX レンダラ 初期化完了\n\n");
}

dx_renderer::~dx_renderer()
{
	if (wav)
		delete wav;

	uninit_dd();
	uninit_ds();
	uninit_di();
	output_log("DirectX レンダラ 破棄しました\n\n");
}

void dx_renderer::graphics_record(char *file)
{
	strcpy(graphics_file,file);
	b_graphics=true;
}

void dx_renderer::sound_record(char *file)
{
	if (file){
		b_sound=true;

		WAVEFORMATEX wfx;
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.wBitsPerSample = 16;
		wfx.cbSize = 0;
		wfx.nChannels = 2;
		wfx.nSamplesPerSec = 44100;
		wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		wav=new CWaveSoundWrite;
		wav->Open(file,&wfx);
		snd_size=0;
	}
	else{
		delete wav;
		wav=NULL;
		b_sound=false;
		show_message("sound record stopped              ");
	}
}

void dx_renderer::output_log(char *mes,...)
{
	va_list vl;
	char buf[256];

	va_start(vl,mes);
	vsprintf(buf,mes,vl);

	SendMessage(m_hwnd,WM_OUTLOG,0,(LPARAM)buf);

	va_end(vl);
	return;
}

word dx_renderer::map_color(word gb_col)
{
#define convert
#ifdef convert
	word r,g,b;
	int r2,g2,b2;

	r=((gb_col>>0)&0x1f)<<3;
	g=((gb_col>>5)&0x1f)<<3;
	b=((gb_col>>10)&0x1f)<<3;

	r2=m_filter.r_def+((r*m_filter.r_r+g*m_filter.r_g+b*m_filter.r_b)/((!m_filter.r_div)?1:m_filter.r_div));
	g2=m_filter.g_def+((r*m_filter.g_r+g*m_filter.g_g+b*m_filter.g_b)/((!m_filter.g_div)?1:m_filter.g_div));
	b2=m_filter.b_def+((r*m_filter.b_r+g*m_filter.b_g+b*m_filter.b_b)/((!m_filter.b_div)?1:m_filter.b_div));

	r2=(r2>255)?255:((r2<0)?0:r2);
	g2=(g2>255)?255:((g2<0)?0:g2);
	b2=(b2>255)?255:((b2<0)?0:b2);

	gb_col=(r2>>3)|((g2>>3)<<5)|((b2>>3)<<10);

#endif
	// xBBBBBGG GGGRRRRR から変換
	if (color_type==0) // ->RRRRRGGG GGxBBBBB に変換 (565)
		return ((gb_col&0x1F)<<11)|((gb_col&0x3e0)<<1)|((gb_col&0x7c00)>>10)|((gb_col&0x8000)>>10);
	if (color_type==1) // ->xRRRRRGG GGGBBBBB に変換 (1555)
		return ((gb_col&0x1F)<<10)|(gb_col&0x3e0)|((gb_col&0x7c00)>>10)|(gb_col&0x8000);
	if (color_type==2) // ->RRRRRGGG GGBBBBBx に変換 (5551)
		return ((gb_col&0x1F)<<11)|((gb_col&0x3e0)<<1)|((gb_col&0x7c00)>>9)|(gb_col>>15);
	else
		return gb_col;
}

word dx_renderer::unmap_color(word gb_col)
{
	// xBBBBBGG GGGRRRRR へ変換
	if (color_type==0) // ->RRRRRGGG GGxBBBBB から変換 (565)
		return (gb_col>>11)|((gb_col&0x7c0)>>1)|(gb_col<<10)|((gb_col&0x40)<<10);
	if (color_type==1) // ->xRRRRRGG GGGBBBBB から変換 (1555)
		return ((gb_col&0x7c00)>>10)|(gb_col&0x3e0)|((gb_col&0x1f)<<10)|(gb_col&0x8000);
	if (color_type==2) // ->RRRRRGGG GGBBBBBx から変換 (5551)
		return (gb_col>>11)|((gb_col&0x7c0)>>1)|((gb_col&0x3e)<<9)|(gb_col<<15);
	else
		return gb_col;
}
/*
static dword convert_to_second(SYSTEMTIME *sys)
{
	dword i,ret=0;
	static int month_days[]={31,28,31,30,31,30,31,31,30,31,30,31};

	for (i=1;i+1950<sys->wYear;i++)
		if ((i&3)==0) // 閏年
			if ((i%100)==0) // 閏年例外
				if ((i%400)==0) // やっぱ閏年
					ret+=366;
				else
					ret+=365;
			else
				ret+=366;
		else
			ret+=365;

	for (i=1;i<sys->wMonth;i++)
		if (i==2)
			if ((sys->wYear&3)==0)
				if ((sys->wYear%100)==0)
					if ((sys->wYear%400)==0)
						ret+=29;
					else
						ret+=28;
				else
					ret+=29;
			else
				ret+=28;
		else
			ret+=month_days[i];

	ret+=sys->wDay-1;

	ret*=24*60*60; // 秒に変換

	ret+=sys->wHour*60*60;
	ret+=sys->wMinute*60;
	ret+=sys->wSecond;

	return ret;
}

static int pathed_seconds(SYSTEMTIME from,SYSTEMTIME to)
{
	return convert_to_second(&to)-convert_to_second(&from);
}
*/
byte dx_renderer::get_time(int type)
{
//	SYSTEMTIME sys;
//	GetSystemTime(&sys);

//	dword now=convert_to_second(&sys);
//	now-=cur_time;
	dword now=(b_time_fix?fixed_time:time(NULL))-cur_time;

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

void dx_renderer::set_time(int type,byte dat)
{
//	SYSTEMTIME sys;
//	GetSystemTime(&sys);

	dword now=(b_time_fix?fixed_time:time(NULL));//convert_to_second(&sys);
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

int dx_renderer::get_timer_state()
{
	return cur_time;
}

void dx_renderer::set_timer_state(int timer)
{
	cur_time=timer;
}

word dx_renderer::get_sensor(bool x_y)
{
	return (x_y?(now_sensor_x&0x0fff):(now_sensor_y&0x0fff));
}

//------------------------------------------------------------

void dx_renderer::init_dd()
{
	if (DirectDrawCreate(NULL,&m_pdd,NULL)!=DD_OK){
		MessageBox(m_hwnd,"Direct Draw オブジェクト生成に失敗しました","レンダラーエラー",MB_OK);
		return;
	}

	GetWindowRect(m_hwnd,&m_bkup);

	init_surface();

	if ((bpp==24)||(bpp==32)){
		for (int i=0;i<0x10000;i++){
			map_24[i]=((i&0xf800)<<8)|((i&0x7c0)<<5)|((i&0x3f)<<2);
		}
	}

	mes_show=false;
	mes[0]='\0';
	mul=1;
}

void dx_renderer::swith_screen_mode()
{
	release_surface();
	b_window=!b_window;
	init_surface();
}

void dx_renderer::set_render_pass(int type)
{
	if ((type>=0)&&(type<=2)&&(render_pass_type!=type)){
		render_pass_type=type;
		release_surface();
		init_surface();
	}
}

void dx_renderer::init_surface()
{
	if (!b_window){
		m_pdd->SetCooperativeLevel(m_hwnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);

		b_640_480=false;

		if (DD_OK!=m_pdd->SetDisplayMode(320,240,16)){
			m_pdd->SetDisplayMode(640,480,16);
			b_640_480=true;
		}

		SetWindowLong(m_hwnd,GWL_STYLE,GetWindowLong(m_hwnd,GWL_STYLE)&~WS_OVERLAPPEDWINDOW|WS_EX_TOPMOST); 
		while(ShowCursor(FALSE)>=0);

		DDSURFACEDESC ddsd;
		memset(&ddsd,0x00,sizeof(ddsd));
		ddsd.dwSize=sizeof(ddsd);
		ddsd.dwFlags=DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
		ddsd.dwBackBufferCount=1;

		if (m_pdd->CreateSurface(&ddsd,&m_pps,NULL)!=DD_OK){
			MessageBox(m_hwnd,"プライマリサーフェースが取得できません","レンダラーエラー",MB_OK);
			return;
		}

		DDSCAPS ddscaps;
		memset(&ddscaps,0x00,sizeof(ddscaps));
		ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
		if (m_pps->GetAttachedSurface(&ddscaps,&m_pbs)!=DD_OK){
			m_pps->Release();
			MessageBox(m_hwnd,"バックサーフェースが取得できません","レンダラーエラー",MB_OK);
			return;
		}

		DDBLTFX ddbfx;
		ddbfx.dwSize=sizeof(ddbfx);
		ddbfx.dwFillColor=0x000000;

		m_pbs->Blt(NULL,NULL,NULL,DDBLT_WAIT|DDBLT_COLORFILL,&ddbfx);
		m_pps->Flip(NULL,DDFLIP_WAIT);

		ddsd.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
		ddsd.dwWidth=160;
		ddsd.dwHeight=144;

		if (render_pass_type==0){
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
			if (m_pdd->CreateSurface(&ddsd,&m_pss,NULL)!=DD_OK){
				m_pps->Release();
				MessageBox(m_hwnd,"バックサーフェースが作成できません","レンダラーエラー",MB_OK);
				return;
			}
			m_pss2=NULL;
		}
		else if (render_pass_type==1){
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
			if (m_pdd->CreateSurface(&ddsd,&m_pss,NULL)!=DD_OK){
				m_pps->Release();
				MessageBox(m_hwnd,"バックサーフェースが作成できません","レンダラーエラー",MB_OK);
				return;
			}
			m_pss2=NULL;
		}
		else if (render_pass_type==2){
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
			if (m_pdd->CreateSurface(&ddsd,&m_pss,NULL)!=DD_OK){
				m_pps->Release();
				MessageBox(m_hwnd,"バックサーフェースが作成できません","レンダラーエラー",MB_OK);
				return;
			}
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
			if (m_pdd->CreateSurface(&ddsd,&m_pss2,NULL)!=DD_OK){
				m_pps->Release();
				MessageBox(m_hwnd,"バックサーフェースが作成できません","レンダラーエラー",MB_OK);
				return;
			}
		}
	}
	else{
		m_pdd->SetCooperativeLevel(m_hwnd,DDSCL_NORMAL);
		m_pdd->RestoreDisplayMode();
		while(ShowCursor(TRUE)<0);

		SetWindowLong(m_hwnd,GWL_STYLE,GetWindowLong(m_hwnd,GWL_STYLE)|WS_OVERLAPPEDWINDOW); 
		SetWindowPos(m_hwnd,HWND_NOTOPMOST,m_bkup.left,m_bkup.top,(m_bkup.right-m_bkup.left),(m_bkup.bottom-m_bkup.top),SWP_SHOWWINDOW);
		on_move();

		DDSURFACEDESC ddsd;
		ZeroMemory(&ddsd,sizeof(ddsd));
		ddsd.dwSize=sizeof(ddsd);
		ddsd.dwFlags=DDSD_CAPS;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;

		if (m_pdd->CreateSurface(&ddsd,&m_pps,NULL)!=DD_OK){
			MessageBox(m_hwnd,"プライマリサーフェースが取得できません","レンダラーエラー",MB_OK);
			return;
		}

		if (m_pdd->CreateClipper(0,&m_pclip,NULL)!=DD_OK){
			m_pps->Release();
			MessageBox(m_hwnd,"クリッパが作成できません","レンダラーエラー",MB_OK);
			return;
		}

		m_pclip->SetHWnd(0,m_hwnd);
		m_pps->SetClipper(m_pclip);
		m_pclip->Release();
		m_pclip=NULL;

		ddsd.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
		ddsd.dwWidth=160;
		ddsd.dwHeight=144;
		if (render_pass_type==0){
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;

			if (m_pdd->CreateSurface(&ddsd,&m_pss,NULL)!=DD_OK){
				m_pps->Release();
				return;
			}
			m_pss2=NULL;
		}
		else if (render_pass_type==1){
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
			if (m_pdd->CreateSurface(&ddsd,&m_pss,NULL)!=DD_OK){
				m_pps->Release();
				return;
			}
			m_pss2=NULL;
		}
		else if (render_pass_type==2){
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
			if (m_pdd->CreateSurface(&ddsd,&m_pss,NULL)!=DD_OK){
				m_pps->Release();
				return;
			}
			ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
			if (m_pdd->CreateSurface(&ddsd,&m_pss2,NULL)!=DD_OK){
				m_pps->Release();
				return;
			}
		}

		m_pbs=NULL;
	}

	DDSURFACEDESC _ddsd;
	_ddsd.dwSize=sizeof(_ddsd);
	_ddsd.dwFlags=DDSD_PIXELFORMAT;
	m_pdd->GetDisplayMode(&_ddsd);
	bpp=_ddsd.ddpfPixelFormat.dwRGBBitCount;

	ZeroMemory(&_ddsd,sizeof(_ddsd));
	_ddsd.dwSize=sizeof(_ddsd);
	m_pps->GetSurfaceDesc(&_ddsd);

	if (_ddsd.ddpfPixelFormat.dwGBitMask==0x7E0)
		color_type=0;
	else if (_ddsd.ddpfPixelFormat.dwGBitMask==0x3E0)
		color_type=1;
	else
		color_type=2;

	if ((bpp!=16)&&(bpp!=15))
		color_type=2;
}

void dx_renderer::uninit_dd()
{
	release_surface();
	m_pdd->Release();
}

void dx_renderer::release_surface()
{
	if (m_pss2){
		m_pss2->Release();
		m_pss2=NULL;
	}
	m_pss->Release();
	m_pps->Release();
}

void dx_renderer::on_move()
{
	GetWindowRect(m_hwnd,&m_window);
	GetClientRect(m_hwnd,&m_viewport);
	GetClientRect(m_hwnd,&m_screen);
	ClientToScreen(m_hwnd,(POINT*)&m_screen.left);
	ClientToScreen(m_hwnd,(POINT*)&m_screen.right);
}

void dx_renderer::draw_menu(int n)
{
	if (n){
		m_pdd->FlipToGDISurface();
		DrawMenuBar(m_hwnd);
	}
	else{
		DDBLTFX ddbfx;
		ddbfx.dwSize=sizeof(ddbfx);
		ddbfx.dwFillColor=0x000000;
		m_pps->Blt(NULL,NULL,NULL,DDBLT_COLORFILL,&ddbfx);
	}
}

void dx_renderer::render_screen(byte *buf,int width,int height,int depth)
{
	int i,j;
	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd,sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	m_pss->Lock(NULL,&ddsd,DDLOCK_WAIT|DDLOCK_WRITEONLY|DDLOCK_SURFACEMEMORYPTR,NULL);

	WORD *sdat=(WORD*)ddsd.lpSurface;
	int pitch=ddsd.lPitch/2;

	if ((bpp!=24)&&(bpp!=32))
		for (i=0;i<height;i++)
			memcpy(sdat+(i*pitch),buf+(i*width*depth/8),width*depth/8);
	else{
		if (bpp==32){
			DWORD *ddat=(DWORD*)sdat,*spare=ddat;
			WORD *wbuf=(WORD*)buf;
			int dpitch=ddsd.lPitch/4;
			for (i=0;i<height;i++){
				for (j=0;j<width;j++){
					*(ddat++)=map_24[*(wbuf++)];
				}
				spare+=dpitch;
				ddat=spare;
			}
		}
		else if (bpp==24){
			BYTE *ddat=(BYTE*)sdat;
			BYTE *spare=ddat;
			WORD *wbuf=(WORD*)buf;
			int dpitch=ddsd.lPitch;
			for (i=0;i<height;i++){
				for (j=0;j<width;j++){
					*((DWORD*)ddat)=map_24[*(wbuf++)];
					ddat+=3;
				}
				spare+=dpitch;
				ddat=spare;
			}
		}
	}

	m_pss->Unlock(NULL);

	if (b_graphics){
		FILE *file=fopen(graphics_file,"wb");
		BITMAPFILEHEADER bmf;
		BITMAPINFOHEADER bmi;
		DWORD mask[3];
		
		ZeroMemory(&bmf,sizeof(bmf));
		ZeroMemory(&bmi,sizeof(bmi));

		bmf.bfSize=sizeof(bmf)+sizeof(bmi)+160*144*2;//+3*sizeof(DWORD);
		bmf.bfOffBits=sizeof(bmf)+sizeof(bmi);//+3*sizeof(DWORD);
		bmf.bfType='B'|('M'<<8);

		bmi.biSize=sizeof(bmi);
		bmi.biBitCount=16;
		bmi.biWidth=160;
		bmi.biHeight=144;
		bmi.biCompression=BI_RGB;
		bmi.biSizeImage=160*144*2;
		bmi.biPlanes=1;

		mask[0]=0x001F;
		mask[1]=0x03E0;
		mask[2]=0x7C00;

		fwrite(&bmf,sizeof(bmf),1,file);
		fwrite(&bmi,sizeof(bmi),1,file);

		WORD tmp;
		for (i=0;i<144;i++){
			for (j=0;j<160;j++){
				tmp=*((WORD*)buf+j+(144-i-1)*160);
				tmp=unmap_color(tmp);
				tmp=((tmp&0x1f)<<10)|(tmp&0x3e0)|((tmp>>10)&0x1f);
				fwrite(&tmp,sizeof(WORD),1,file);
			}
		}
		fclose(file);

		show_message("save screenshot");

		b_graphics=false;
	}

	flip(true);
}

void dx_renderer::flip(bool b_software)
{
	static long beforetime=timeGetTime();
	static int count=0,beforecount=0;
	HDC hdc;
	char buf[256];

	if (timeGetTime()-beforetime>1000){
		beforecount=count;
		count=0;
		beforetime=timeGetTime();
	}

	sprintf(buf,"%4d  ",beforecount*mul);

	if (b_window){
		if (b_showfps||mes_show){
			m_pss->GetDC(&hdc);
			SetTextColor(hdc,RGB(255,0,0));
			SetBkMode(hdc,TRANSPARENT);

			if (b_showfps)
				TextOut(hdc,125,3,buf,strlen(buf));
			if (mes_show){
				TextOut(hdc,2,125,mes,strlen(mes));
				if ((timeGetTime()-mes_start)>2000)
					mes_show=false;
			}
			m_pss->ReleaseDC(hdc);
		}
	}
	else{
		if (b_showfps||mes_show){
			m_pps->GetDC(&hdc);
			SetTextColor(hdc,RGB(255,255,0));
			SetBkColor(hdc,RGB(0,0,0));
			if (b_showfps)
				TextOut(hdc,320*(b_640_480?2:1)-(160-125),3,buf,strlen(buf));
			if (mes_show){
				if ((timeGetTime()-mes_start)>2000){
					TextOut(hdc,2,240*(b_640_480?2:1)-(144-125),"                                          ",40);
					mes_show=false;
				}
				else
					TextOut(hdc,2,240*(b_640_480?2:1)-(144-125),mes,strlen(mes));
			}
			m_pps->ReleaseDC(hdc);
		}
	}

	RECT rect={0,0,160,144};

	if (vsync) m_pdd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,NULL);

#define BLT_ASYNC

	if (b_window){
#ifndef BLT_ASYNC
		if (render_pass_type==0){
			m_pps->Blt(&m_screen,m_pss,&rect,DDBLT_WAIT,NULL);
		}
		else if (render_pass_type==1){
			m_pps->Blt(&m_screen,m_pss,&rect,DDBLT_WAIT,NULL);
		}
		else if (render_pass_type==2){
			m_pss2->Blt(NULL,m_pss,NULL,DDBLT_WAIT,NULL);
			m_pps->Blt(&m_screen,m_pss2,&rect,DDBLT_WAIT,NULL);
		}
#else
		if (render_pass_type==0){
			m_pps->Blt(&m_screen,m_pss,&rect,DDBLT_ASYNC,NULL);
		}
		else if (render_pass_type==1){
			m_pps->Blt(&m_screen,m_pss,&rect,DDBLT_ASYNC,NULL);
		}
		else if (render_pass_type==2){
			m_pss2->Blt(NULL,m_pss,NULL,DDBLT_ASYNC,NULL);
			m_pps->Blt(&m_screen,m_pss2,&rect,DDBLT_ASYNC,NULL);
		}
#endif
	}
	else{
		RECT rect2;
		if (b_640_480){
			rect2.left=320-160;
			rect2.top=240-144;
			rect2.right=320+160;
			rect2.bottom=240+144;
		}
		else{
			rect2.left=160-80;
			rect2.top=120-72;
			rect2.right=160+80;
			rect2.bottom=120+72;
		}
#ifndef BLT_ASYNC
		if (render_pass_type==0){
			m_pps->Blt(&rect2,m_pss,&rect,DDBLT_WAIT,NULL);
		}
		else if (render_pass_type==1){
			m_pps->Blt(&rect2,m_pss,&rect,DDBLT_WAIT,NULL);
		}
		else if (render_pass_type==2){
			m_pss2->Blt(NULL,m_pss,NULL,DDBLT_WAIT,NULL);
			m_pps->Blt(&rect2,m_pss2,&rect,DDBLT_WAIT,NULL);
		}
#else
		if (render_pass_type==0){
			m_pps->Blt(&rect2,m_pss,&rect,DDBLT_ASYNC,NULL);
		}
		else if (render_pass_type==1){
			m_pps->Blt(&rect2,m_pss,&rect,DDBLT_ASYNC,NULL);
		}
		else if (render_pass_type==2){
			m_pss2->Blt(NULL,m_pss,NULL,DDBLT_ASYNC,NULL);
			m_pps->Blt(&rect2,m_pss2,&rect,DDBLT_ASYNC,NULL);
		}
#endif
//			m_pps->Blt(&rect2,m_pss,&rect,DDBLTF_WAIT,NULL);
			/*
#ifndef BLT_ASYNC
			if (render_pass_type==0){
				m_pps->BltFast((320-160)/2,(240-144)/2,m_pss,&rect,DDBLTFAST_WAIT);
			}
			else if (render_pass_type==1){
				m_pps->BltFast((320-160)/2,(240-144)/2,m_pss,&rect,DDBLTFAST_WAIT);
			}
			else if (render_pass_type==2){
				m_pss2->BltFast(0,0,m_pss,NULL,DDBLTFAST_WAIT);
				m_pps->BltFast((320-160)/2,(240-144)/2,m_pss2,&rect,DDBLTFAST_WAIT);
			}
#else
			if (render_pass_type==0){
				m_pps->BltFast((320-160)/2,(240-144)/2,m_pss,&rect,DDBLTFAST_NOCOLORKEY);
			}
			else if (render_pass_type==1){
				m_pps->BltFast((320-160)/2,(240-144)/2,m_pss,&rect,DDBLTFAST_NOCOLORKEY);
			}
			else if (render_pass_type==2){
				m_pss2->BltFast(0,0,m_pss,NULL,DDBLTFAST_WAIT);
				m_pps->BltFast((320-160)/2,(240-144)/2,m_pss2,&rect,DDBLTFAST_NOCOLORKEY);
			}
#endif
//			m_pps->BltFast((320-160)/2,(240-144)/2,m_pss,&rect,DDBLTFAST_WAIT);
		}*/
	}

	count++;
}

void dx_renderer::show_message(char *message)
{
	strcpy(mes,message);
	mes_show=true;
	mes_start=timeGetTime();
}

//----------------------------------------------

#define BUF_LEN 160

void dx_renderer::init_ds()
{
	DSBUFFERDESC dsbd;

	DirectSoundCreate(NULL,&m_pds,NULL);
	m_pds->SetCooperativeLevel(m_hwnd,DSSCL_PRIORITY);

	dsbd.dwBufferBytes=0;
	dsbd.dwReserved=0;
	dsbd.lpwfxFormat=NULL;
	dsbd.dwSize=sizeof(dsbd);
	dsbd.dwFlags=DSBCAPS_PRIMARYBUFFER;

	m_pds->CreateSoundBuffer(&dsbd,&m_ppb,NULL);

	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.wBitsPerSample = 16;
	wfx.cbSize = 0;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	m_ppb->SetFormat(&wfx);

	dsbd.dwReserved=0;
	dsbd.lpwfxFormat=&wfx;
	dsbd.dwSize=sizeof(dsbd);
	dsbd.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY;

	dsbd.dwBufferBytes=wfx.nAvgBytesPerSec*BUF_LEN/1000;
	m_pds->CreateSoundBuffer(&dsbd,&m_pmix,NULL);

	int size=44100*16/8*2*BUF_LEN/1000;
	void *p1,*p2;
	DWORD s1,s2;
	m_pmix->Lock(0,size,&p1,&s1,&p2,&s2,0);
	memset(p1,0,size);
	m_pmix->Unlock(p1,s1,p2,s2);

	snd_render=NULL;

	m_pmix->Play(0,0,DSBPLAY_LOOPING);
}

void dx_renderer::uninit_ds()
{
	m_pmix->Release();
	m_pds->Release();
}

void dx_renderer::pause_sound()
{
	m_pmix->Stop();
	static int size=44100*16/8*2*BUF_LEN/1000;
	void *p1,*p2;
	DWORD s1,s2;
	m_pmix->Lock(0,size,&p1,&s1,&p2,&s2,0);
	memset(p1,0,size);
	m_pmix->Unlock(p1,s1,p2,s2);
}

void dx_renderer::resume_sound()
{
	m_pmix->Play(0,0,DSBPLAY_LOOPING);
}

void dx_renderer::update_sound()
{

	static int size=44100*16/8*2*BUF_LEN/1000;
	static DWORD bef=1;
	DWORD cur_play,start;
	void *p1,*p2;
	DWORD s1,s2;

	m_pmix->GetCurrentPosition(&cur_play,NULL); //後ろのはいらない

	if (bef!=(cur_play*2/size)){
		bef=(cur_play*2/size);

		start=(!bef)?size/2:0;

		m_pmix->Lock(start,size/2,&p1,&s1,&p2,&s2,0); // 循環することはありえない

		if (!snd_render)
			memset(p1,0,size/2);
		else{
			snd_render->render((short*)p1,size/8);
			if (b_sound){
				unsigned int dm;
				char buf[256];
				wav->Write(size/2,(BYTE*)p1,&dm);
				snd_size+=size/2;
				sprintf(buf,"recording %d kb            ",snd_size/1024);
				show_message(buf);
			}
		}
		
		m_pmix->Unlock(p1,s1,p2,s2);
	}
}

//---------------------------------------------

struct callback_dat{
	LPDIRECTINPUT lpdi;
	LPDIRECTINPUTDEVICE *lpdev;
	int *devs;
};

void dx_renderer::init_di()
{
	DirectInputCreate(m_hinst,DIRECTINPUT_VERSION,&m_pdi,NULL);
	m_pdi->CreateDevice(GUID_SysKeyboard,&m_pkeyboad,NULL);
	m_pkeyboad->SetDataFormat(&c_dfDIKeyboard);
	m_pkeyboad->SetCooperativeLevel(m_hwnd,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
	m_pkeyboad->Acquire();
	m_pdi->CreateDevice(GUID_SysMouse,&m_pmouse,NULL);
	m_pmouse->SetDataFormat(&c_dfDIMouse);
	m_pmouse->SetCooperativeLevel(m_hwnd,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
	DIPROPDWORD dipp={{sizeof(DIPROPDWORD),sizeof(DIPROPHEADER),0,DIPH_DEVICE,},16};
	m_pmouse->SetProperty(DIPROP_BUFFERSIZE,&dipp.diph);
	m_pmouse->Acquire();
	
	joysticks=0;
	callback_dat cdat;
	cdat.lpdi=m_pdi;
	cdat.lpdev=m_pjoystick;
	cdat.devs=&joysticks;
	m_pdi->EnumDevices(DIDEVTYPE_JOYSTICK,pad_callback,&cdat,DIEDFL_ATTACHEDONLY/*DIEDFL_ALLDEVICES*/);
	b_can_use_ffb=false;

	for (int i=0;i<joysticks;i++){
		m_pjoystick[i]->SetDataFormat(&c_dfDIJoystick);
		m_pjoystick[i]->SetCooperativeLevel(m_hwnd,DISCL_BACKGROUND|DISCL_EXCLUSIVE);

		DIPROPDWORD	pw;
		DIPROPRANGE	pr;
		pw.diph.dwSize=sizeof(DIPROPDWORD);
		pr.diph.dwSize=sizeof(DIPROPRANGE);
		pw.diph.dwHeaderSize=pr.diph.dwHeaderSize=sizeof(DIPROPHEADER);
		pw.diph.dwHow=pr.diph.dwHow = DIPH_BYOFFSET;
		pw.dwData=5000;
		pr.lMin=-100;
		pr.lMax=+100;
		/* Ｘ軸の設定 */
		pw.diph.dwObj = pr.diph.dwObj = DIJOFS_X;
		m_pjoystick[i]->SetProperty(DIPROP_RANGE,&pr.diph);
//		m_pjoystick[i]->SetProperty(DIPROP_DEADZONE,&pw.diph);
		/* Ｙ軸の設定 */
		pw.diph.dwObj = pr.diph.dwObj = DIJOFS_Y;
		m_pjoystick[i]->SetProperty(DIPROP_RANGE,&pr.diph);
//		m_pjoystick[i]->SetProperty(DIPROP_DEADZONE,&pw.diph);

		m_pjoystick[i]->Acquire();
		m_pjoystick[i]->QueryInterface(IID_IDirectInputDevice2,(void**)&m_pjoystick2[i]);

		DIJOYSTATE js[16];
		m_pjoystick2[i]->Poll();
		m_pjoystick2[i]->GetDeviceState(sizeof(DIJOYSTATE),(void*)&js[i]);
		joy_center[i].x=js[i].lX;
		joy_center[i].y=js[i].lY;
	}

	if (joysticks){
		DWORD    rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
		LONG     rglDirection[2] = { 0, 0 };
		DICONSTANTFORCE cf;
		cf.lMagnitude=10000;
		DIEFFECT eff;
		ZeroMemory( &eff, sizeof(eff) );
		eff.dwSize                  = sizeof(DIEFFECT);
		eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		eff.dwDuration              = INFINITE;
		eff.dwSamplePeriod          = 0;
		eff.dwGain                  = DI_FFNOMINALMAX;
		eff.dwTriggerButton         = DIEB_NOTRIGGER;
		eff.dwTriggerRepeatInterval = 0;
		eff.cAxes                   = 2;
		eff.rgdwAxes                = rgdwAxes;
		eff.rglDirection            = rglDirection;
		eff.lpEnvelope              = 0;
		eff.cbTypeSpecificParams    = sizeof(DICONSTANTFORCE);
		eff.lpvTypeSpecificParams   = &cf;
		HRESULT hr=m_pjoystick2[0]->CreateEffect(GUID_ConstantForce,&eff,&m_peffect,NULL);
		if (FAILED(hr))
			b_can_use_ffb=false;
		else
			b_can_use_ffb=true;
	}

	pad_state=0;
	memset(&key_config,0,sizeof(key_config));
	bool b_auto=false;
}

BOOL CALLBACK dx_renderer::pad_callback(LPCDIDEVICEINSTANCE pdinst,LPVOID pvRef)
{
	callback_dat *p=(callback_dat*)pvRef;

	if (p->lpdi->CreateDevice(pdinst->guidInstance,&p->lpdev[*(p->devs)],NULL)==DI_OK)
		(*p->devs)++;

	return DIENUM_CONTINUE;
}

void dx_renderer::uninit_di()
{
	m_pkeyboad->Unacquire();
	m_pkeyboad->Release();

	if (b_can_use_ffb)
		m_peffect->Release();

	for (int i=0;i<joysticks;i++){
		m_pjoystick2[i]->Release();
		m_pjoystick[i]->Unacquire();
		m_pjoystick[i]->Release();
	}
	m_pdi->Release();
}

void dx_renderer::set_key(key_dat *keys)
{
	memcpy(key_config,keys,sizeof(key_dat)*8);
}

void dx_renderer::set_koro_key(key_dat *keys)
{
	memcpy(koro_key_config,keys,sizeof(key_dat)*4);
}

void dx_renderer::set_koro_analog(bool ana)
{
	b_koro_analog=ana;
}

void dx_renderer::set_koro_sensitivity(int sence)
{
	koro_sence=sence;
}

void dx_renderer::set_bibrate(bool bibrate)
{
	b_bibrating=bibrate;
	if (!(b_use_ffb&&b_can_use_ffb))
		return;

	if (bibrate){
		m_peffect->Start(1,0);
		LONG rglDirection[2] = { 0, 0 };

	    DICONSTANTFORCE cf;
	    cf.lMagnitude = 10000;

		DIEFFECT eff;
		ZeroMemory( &eff, sizeof(eff) );
		eff.dwSize                = sizeof(DIEFFECT);
		eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		eff.cAxes                 = 2;
		eff.rglDirection          = rglDirection;
		eff.lpEnvelope            = 0;
		eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE);
		eff.lpvTypeSpecificParams = &cf;
//		eff.dwStartDelay          = 0;

		// Now set the new parameters and start the effect immediately.
		m_peffect->SetParameters(&eff,DIEP_DIRECTION|DIEP_TYPESPECIFICPARAMS|DIEP_START);
	}
	else
		m_peffect->Stop();
}

void dx_renderer::set_use_ffb(bool use)
{
	b_use_ffb=use;
}

int dx_renderer::check_pad()
{
	return pad_state;
}

void dx_renderer::set_pad(int stat)
{
	pad_state=stat;
}

void dx_renderer::disable_check_pad()
{
	b_pad_update=false;
}

void dx_renderer::enable_check_pad()
{
	b_pad_update=true;
}

void dx_renderer::set_time_fix(bool fix)
{
	b_time_fix=fix;
}

void dx_renderer::set_fixed_time(dword time)
{
	fixed_time=time;
}

void dx_renderer::toggle_auto()
{
	b_auto=!b_auto;
	show_message(b_auto?"auto fire enabled":"auto fire disabled");
}

void dx_renderer::movie_play_start(vector<mov_key> *list)
{
	movie_playing=true;
	movie_start=0;
	mov_cur_pos=0;

	key_list.clear();
	vector<mov_key>::iterator ite;
	for (ite=list->begin();ite!=list->end();ite++)
		key_list.push_back(*ite);
}

void dx_renderer::update_pad()
{
	static bool phase=false;
	static int key_dat_buf=0;

	int i;

	m_pkeyboad->GetDeviceState(sizeof(key_state),(void*)key_state);
	for (i=0;i<joysticks;i++){
		m_pjoystick2[i]->Poll();
		m_pjoystick2[i]->GetDeviceState(sizeof(DIJOYSTATE),(void*)&js[i]);
		js[i].lX-=joy_center[i].x;
		js[i].lY-=joy_center[i].y;
	}

	if (movie_playing){
		if (key_list[mov_cur_pos].frame==movie_start){
			if (key_list[mov_cur_pos].key_code==0xffffffff){
				movie_playing=false;
				show_message("movie ended ...");
			}
			else{
				key_dat_buf=key_list[mov_cur_pos].key_code;
				mov_cur_pos++;
			}
		}
		pad_state=key_dat_buf;
		if (movie_start==0)
			movie_start=1;
		else
			movie_start++;
		return;
	}

	pad_state=0;

	if (b_auto){
		if (phase)
			for (i=0;i<2;i++)
				pad_state|=check_press(key_config+i)?(1<<i):0;
		for (i=2;i<8;i++)
			pad_state|=check_press(key_config+i)?(1<<i):0;
	}
	else
		for (i=0;i<8;i++)
			pad_state|=check_press(key_config+i)?(1<<i):0;

	phase=!phase;

#define MAX_VAL 2047+150
#define MIN_VAL 2047-150

	// モーションセンサー
	if (!b_koro_analog){
		if (check_press(koro_key_config+2)){
			now_sensor_x+=3;
			if (now_sensor_x>MAX_VAL) now_sensor_x=MAX_VAL;
			if (now_sensor_x<2047) now_sensor_x=2047+10;
		}
		else if (check_press(koro_key_config+3)){
			now_sensor_x-=3;
			if (now_sensor_x<MIN_VAL) now_sensor_x=MIN_VAL;
			if (now_sensor_x>2047) now_sensor_x=2047-10;
		}
		else if (now_sensor_x>2047){
			now_sensor_x-=2;
			if (now_sensor_x<2047) now_sensor_x=2047;
		}
		else{
			now_sensor_x+=2;
			if (now_sensor_x>2047) now_sensor_x=2047;
		}

		if (check_press(koro_key_config+0)){
			now_sensor_y+=3;
			if (now_sensor_y>MAX_VAL) now_sensor_y=MAX_VAL;
			if (now_sensor_y<2047) now_sensor_y=2047+10;
		}
		else if (check_press(koro_key_config+1)){
			now_sensor_y-=3;
			if (now_sensor_y<MIN_VAL) now_sensor_y=MIN_VAL;
			if (now_sensor_y>2047) now_sensor_y=2047-10;
		}
		else if (now_sensor_y>2047){
			now_sensor_y-=2;
			if (now_sensor_y<2047) now_sensor_y=2047;
		}
		else{
			now_sensor_y+=2;
			if (now_sensor_y>2047) now_sensor_y=2047;
		}
	}
	else{
		now_sensor_x=-js[0].lX*koro_sence/100+2047;
		now_sensor_y=-js[0].lY*koro_sence/100+2047;
	}

	if (movie_recording){
		int tmp_m;
		if ((movie_start==0)||(key_dat_buf!=pad_state)){
			key_dat_buf=pad_state;
			tmp_m=movie_start;
			fwrite(&tmp_m,4,1,movie_file);
			tmp_m=key_dat_buf;
			fwrite(&tmp_m,4,1,movie_file);
		}
		movie_start++;
	}
}

word dx_renderer::get_any_key()
{
	int i,j;

	m_pkeyboad->GetDeviceState(sizeof(key_state),(void*)key_state);
	for (i=0;i<joysticks;i++){
		m_pjoystick2[i]->Poll();
		m_pjoystick2[i]->GetDeviceState(sizeof(DIJOYSTATE),(void*)&js[i]);
		js[i].lX-=joy_center[i].x;
		js[i].lY-=joy_center[i].y;
	}

	for (i=0;i<256;i++)
		if ((key_state[i]&0x80)&&(i!=0x94)&&(i!=0x3a))
			return i|0x100;

	for (j=0;j<joysticks;j++)
		for (i=0;i<16;i++)
			if (js[j].rgbButtons[i]&0x80)
				return i|((DI_PAD_X+j*NEXT_PAD+2)<<8);

	for (j=0;j<joysticks;j++)
		if ((js[j].lX>50)||(js[j].lX<-50))
			return ((js[j].lX<-50)?1:0)|((DI_PAD_X+j*3)<<8);

	for (j=0;j<joysticks;j++)
		if ((js[j].lY>50)||(js[j].lY<-50))
			return ((js[j].lY<-50)?1:0)|((DI_PAD_X+j*3+1)<<8);

	return 0;
}

bool dx_renderer::check_press(key_dat *dat)
{
	int pad_id,pad_dir;

	if(dat->device_type==DI_KEYBOARD){ // キーボード
		return (key_state[dat->key_code]&0x80)?true:false;
	}
	else if (dat->device_type>=DI_PAD_X){ // ジョイスティック
		pad_id=(dat->device_type-DI_PAD_X)/NEXT_PAD;
		pad_dir=(dat->device_type-DI_PAD_X)%NEXT_PAD;
		if (pad_dir==0){ // X軸
			if (!dat->key_code) // 正方向
				return (js[pad_id].lX>50)?true:false;
			else // 負方向
				return (js[pad_id].lX<-50)?true:false;
		}
		else if (pad_dir==1){ // Y軸
			if (!dat->key_code) // 正方向
				return (js[pad_id].lY>50)?true:false;
			else // 負方向
				return (js[pad_id].lY<-50)?true:false;
		}
		else if (pad_dir==2){ // ボタン
			return (js[pad_id].rgbButtons[dat->key_code]&0x80)?true:false;
		}
	}
	return false;
}

void dx_renderer::refresh()
{
	static bool bef_f5=false,bef_f7=false,bef_auto=false,bef_pause=false;

	if (b_pad_update||movie_playing||movie_recording)
		update_pad();
	update_sound();

	if ((!bef_f5&&check_press(&save_key))||(save_resurve!=-1)){
		SendMessage(m_hwnd,WM_COMMAND,MAKEWPARAM(ID_SAVE_STATE,0),save_resurve);
		save_resurve=-1;
	}
	else if ((!bef_f7&&check_press(&load_key))||(load_resurve!=-1)){
		SendMessage(m_hwnd,WM_COMMAND,MAKEWPARAM(ID_RESTORE_STATE,0),load_resurve);
		load_resurve=-1;
	}
	else if (!bef_auto&&check_press(&auto_key))
		toggle_auto();
	else if (!bef_pause&&check_press(&pause_key))
		SendMessage(m_hwnd,WM_COMMAND,MAKEWPARAM(ID_PAUSE,0),0);

	bef_f5=check_press(&save_key)?true:false;
	bef_f7=check_press(&load_key)?true:false;
	bef_auto=check_press(&auto_key)?true:false;
	bef_pause=check_press(&pause_key)?true:false;
}
