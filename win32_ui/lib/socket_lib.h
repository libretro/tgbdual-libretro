/*--------------------------------------------------
   TGB Dual - Gameboy Emulator -
   Copyright (C) 2004  Hii

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

#pragma once

#include <winsock.h>
#include <string>
#include <list>

using namespace std;

class socket_exception{
public:
	socket_exception():msg(""){
	}
	socket_exception(char *p, ...){
		va_list vl;
		va_start(vl,p);
		init(p,vl);
		va_end(vl);
	}
	socket_exception(string &s, ...){
		va_list vl;
		va_start(vl,s);
		init(s.c_str(),vl);
		va_end(vl);
	}

	string &get_msg(){
		return msg;
	}
private:
	void init(const char *p,va_list vl){
		static char buf[256];
		vsprintf(buf,p,vl);
		msg=string(buf);
	}
	string msg;
};

class winsock_obj{
public:
	static winsock_obj *get_instance(){
		if (ref) return ref;
		else{
			ref=new winsock_obj();
			return ref;
		}
	};
	// これ、呼び出さしたら即deleteされる
	static void delete_instance(){
		if (ref){
			delete ref;
			ref=NULL;
		}
	};

	list<string> get_ipaddrs(){
		list<string> ret;
		char ac[80];
		gethostname(ac,sizeof(ac));

		hostent *he=gethostbyname(ac);
		for (int i=0;he->h_addr_list[i];i++){
			in_addr addr;
			memcpy(&addr,he->h_addr_list[i],sizeof(in_addr));
			ret.push_back(string(inet_ntoa(addr)));
		}
		return ret;
	}

private:
	static winsock_obj *ref;

	winsock_obj(){
		WSADATA wsa;
		WSAStartup(MAKEWORD(2,0),&wsa);
	};
	~winsock_obj(){
		WSACleanup();
	}
};

class winsock_initializer{
public:
	winsock_initializer(){
		winsock_obj::get_instance();
	}
	~winsock_initializer(){
		winsock_obj::delete_instance();
	}
};

class socket_obj{
public:
	socket_obj(string host,int port){
		winsock_obj::get_instance();
		s=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

		sockaddr_in addr;
		hostent *he;
		addr.sin_family=AF_INET;
		addr.sin_port=htons(port);
		he=gethostbyname(host.c_str());
		*(int*)&addr.sin_addr.s_addr=*(int*)he->h_addr_list[0];

		if (connect(s,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR)
			valid=false;
	}
	socket_obj(SOCKET s){
		this->s=s;
		if (s!=INVALID_SOCKET)
			valid=true;
	}
	~socket_obj(){
		if (s!=INVALID_SOCKET)
			closesocket(s);
	}

	bool connected(){
		return valid&&s!=INVALID_SOCKET; // やや冗長であるが
	}
	bool ready_to_receive(){
		if (connected()){
			fd_set fs;
			FD_ZERO(&fs);
			FD_SET(s,&fs);
			timeval tv={0};
			select(s+1,&fs,NULL,NULL,&tv);
			return !!(FD_ISSET(s,&fs));
		}
		else return false;
	}
	void set_no_delay(bool b){
		if (connected()){
			// Nagleアルゴリズムを無効にする
			int t=b?1:0;
			setsockopt(s,IPPROTO_TCP,TCP_NODELAY,(const char *)&t,sizeof(t));
		}
	}

	void send(void *dat,int size){ // throws socket_exception
		if (connected())
			::send(s,(const char *)dat,size,0);
		else
			throw socket_exception("Socket Disconnected.");
	}
	int recv(void *buf,int size){
		return ::recv(s,(char*)buf,size,0);
	}

	// きっかりsizeだけ読み込む。終わるまでブロックする。
	void read(void *buf,int size){ // throws socket_exception
		for (int recieved=0;recieved!=size;){
			int len=recv((char*)buf+recieved,size-recieved);
			if (len==0){
				// 接続が切れているとき
				valid=false;
				closesocket(s);
				s=INVALID_SOCKET;
				throw socket_exception("Socket Disconnected.");
			}
			recieved+=len;
		}
	}

private:
	SOCKET s;
	bool valid;
};

class server_socket{
public:
	server_socket(int port){
		s=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		sockaddr_in addr;
		addr.sin_family=AF_INET;
		addr.sin_port=htons(port);
		addr.sin_addr.s_addr=htonl(INADDR_ANY);
		bind(s,(sockaddr*)&addr,sizeof(addr));
		listen(s,1);
	}
	~server_socket(){
		closesocket(s);
	}

	socket_obj *accept(){
		sockaddr_in addr;
		int len=sizeof(addr);
		SOCKET a=::accept(s,(sockaddr*)&addr,&len);
		return new socket_obj(a);
	}
private:
	SOCKET s;
};
