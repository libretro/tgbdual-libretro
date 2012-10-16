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

/*-------------------------------------------------------------
          Winsock TCP/IP Wrapper Copyright(C) Hii 2000
-------------------------------------------------------------*/

#include <windows.h>
#include <winsock.h>

#define WM_OUTLOG WM_USER+100
#define WM_SOCKET WM_USER+200

class sock
{
public:
	sock(HWND hwnd);
	~sock();

	bool init(bool b_serv);
	void uninit();
	void connect_server(char *ip_addr);

	int send(BYTE *dat,int len);
	int recv(BYTE *dat);
	int direct_recv(BYTE *dat);

	void set_blocking(bool block);

	bool is_server() { return b_server; }
	bool is_connected() { return b_connected; }

	void handle_message(WPARAM wParam,LPARAM lParam);
	void get_my_addr(char *buf);

	void out_log(char *mes,...);

private:
	bool b_server;
	bool b_connected;
	bool b_blocking;

	SOCKET listen_sock;
	SOCKET target_sock;

	BYTE buf[256];
	int message_size;

	HWND m_hwnd;
};
