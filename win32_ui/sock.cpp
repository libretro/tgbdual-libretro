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

#include "sock.h"
#include <stdio.h>

sock::sock(HWND hwnd)
{
	m_hwnd=hwnd;

	listen_sock=INVALID_SOCKET;
	target_sock=INVALID_SOCKET;

	b_connected=false;
	message_size=0;

	WSADATA wsa;
	WSAStartup(MAKEWORD(1,1),&wsa);
}

sock::~sock()
{
	uninit();
	WSACleanup();
}

void sock::out_log(char *mes,...)
{
	va_list vl;
	char buf[256];

	va_start(vl,mes);
	vsprintf(buf,mes,vl);

	SendMessage(m_hwnd,WM_OUTLOG,0,(LPARAM)buf);

	va_end(vl);
}

void sock::get_my_addr(char *buf)
{
	char tmp[256];
	gethostname(tmp,256);
	hostent *host=gethostbyname(tmp);
	in_addr addr;
	*(int*)&addr.s_addr=*(int*)host->h_addr_list[0];
	strcpy(buf,inet_ntoa(addr));
}

void sock::uninit()
{
	if (listen_sock!=INVALID_SOCKET)
		closesocket(listen_sock);
	if (target_sock!=INVALID_SOCKET)
		closesocket(target_sock);	
	b_connected=false;
	out_log("ソケットをクローズしました\n\n");
}

bool sock::init(bool b_serv)
{
	b_server=b_serv;

	if (b_server){
		if ((listen_sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==INVALID_SOCKET)
			return false;

		sockaddr_in listen_addr;
		listen_addr.sin_family=AF_INET;
		listen_addr.sin_port=htons(6502);
		listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);

		if (bind(listen_sock,(sockaddr*)&listen_addr,sizeof(listen_addr))==SOCKET_ERROR)
			return false;

		if (listen(listen_sock,1)==SOCKET_ERROR)
			return false;

		if (WSAAsyncSelect(listen_sock,m_hwnd,WM_SOCKET,FD_ACCEPT)==SOCKET_ERROR)
			return false;
	}
	else
		if ((target_sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==INVALID_SOCKET)
			return false;

	out_log("Winsock 1.1 初期化完了\n\n");
	if (b_server)
		out_log("クライアントの接続を待っています\n\n");
	return true;
}

void sock::connect_server(char *ip_addr)
{
	if (b_server)
		return;

	sockaddr_in server_addr;
	hostent *host;

	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(6502);

	if (isdigit(ip_addr[0]))
		*(int*)&server_addr.sin_addr.s_addr=inet_addr(ip_addr);
	else{
		host=gethostbyname(ip_addr);
		*(int*)&server_addr.sin_addr.s_addr=*(int*)host->h_addr_list[0];
	}

	WSAAsyncSelect(target_sock,m_hwnd,WM_SOCKET,FD_CONNECT);

	connect(target_sock,(sockaddr*)&server_addr,sizeof(server_addr));
	out_log("サーバーへの接続を開始します\n\n");
}

void sock::set_blocking(bool block)
{
	if (block)
		WSAAsyncSelect(target_sock,m_hwnd,WM_SOCKET,FD_CLOSE);
	else
		WSAAsyncSelect(target_sock,m_hwnd,WM_SOCKET,FD_READ|FD_CLOSE);
}

int sock::send(BYTE *dat,int len)
{
	if(target_sock!=INVALID_SOCKET){
		::send(target_sock,(char*)dat,len,0);
		return len;
	}
	else
		return 0;
}

int sock::recv(BYTE *dat)
{
	int ret=message_size;
	if (message_size){
		memcpy(dat,buf,message_size);
		message_size=0;
	}
	return ret;
}

int sock::direct_recv(BYTE *dat)
{
	return ::recv(target_sock,(char*)buf,256,0);
}

void sock::handle_message(WPARAM wParam,LPARAM lParam)
{
	switch(WSAGETSELECTEVENT(lParam))
	{
	case FD_ACCEPT:
		sockaddr_in target_addr;
		int length;
		length=sizeof(target_addr);
		target_sock=accept(listen_sock,(sockaddr*)&target_addr,&length);
		hostent *host;
		host=gethostbyaddr((char*)&target_addr.sin_addr.s_addr,4,AF_INET);
		if (host)
			out_log("接続確立\nclient : \"%s\"\n\n",host->h_name);
		WSAAsyncSelect(target_sock,m_hwnd,WM_SOCKET,FD_READ|FD_CLOSE);
		b_connected=true;
		break;
	case FD_CONNECT:
		out_log("接続確立\n\n");
		WSAAsyncSelect(target_sock,m_hwnd,WM_SOCKET,FD_READ|FD_CLOSE);
		b_connected=true;
		break;
	case FD_READ:
		message_size=::recv(target_sock,(char*)buf,256,0);
		break;
	case FD_CLOSE:
		out_log("接続切断\n\n");
		b_connected=false;
		uninit();
		break;
	}
}
