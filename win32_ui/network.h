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

// ネットワーク対戦用
#pragma once

#include "./lib/socket_lib.h"
#include "./lib/thread.h"
#include "./lib/critical_section.h"

#include <queue>
#include <string>

using namespace std;

template <class T>
class netplay : public thread{
public:
	typedef T data_type;
	// サーバー用のコンストラクタ
	netplay(int port){
		server=true;
		host="";
		this->port=port;
		my_sram=opp_sram=NULL;
		alive=true;
		prepared=false;
		start();
	}
	// クライアント用のコンストラクタ
	netplay(string &host,int port){
		server=false;
		this->host=host;
		this->port=port;
		my_sram=opp_sram=NULL;
		alive=true;
		prepared=false;
		start();
	}

	~netplay(){
		char *ms=(char*)my_sram,*os=(char*)opp_sram;
		if (my_sram) delete []ms;
		if (opp_sram) delete []os;

		// スレッド終了まで待機
		if (running()){
			if (done_prepare()){
				alive=false;
				join();
			}
			else{
				// 駄目ですな…このまま終わればスレッドをTerminateしてくれるはず
			}
		}
	}

	// サーバー？
	bool is_server(){ return server; }

	// 準備完了している？
	bool done_prepare(){ return prepared; }

	// ネットワーク遅延 往復路
	int network_delay(){ return delay; }

	// 接続中か？
	bool connected(){ return s!=NULL; }

	// SRAMの送受信
	void send_sram(char *data,int size){
		my_sram_size=size;
		char *tmp=new char[size];
		memcpy(tmp,data,size);
		my_sram=tmp;
	}
	pair<char*,int> get_sram(){
		return pair<char*,int>((char*)opp_sram,(int)opp_sram_size);
	}

	// キーデータの送受信
	void send_keydata(data_type &dat){
		try{
			critical_lock cl(cs);
			my_keydata.push(dat);
			send_packet(packet("KEY ",sizeof(data_type),(char*)&dat));
		} catch(socket_exception&){}
	}
	int get_keydata_num(){
		critical_lock cl(cs);
		return min(my_keydata.size(),opp_keydata.size());
	}
	pair<data_type,data_type> pop_keydata(){
		critical_lock cl(cs);
		data_type f=my_keydata.front(),s=opp_keydata.front();
		my_keydata.pop();
		opp_keydata.pop();
		return pair<data_type,data_type>(f,s);
	}

	// チャットデータの送受信
	void send_message(string &msg){
		try{
			critical_lock cl(cs);
			send_packet(packet("MSG ",msg.length()+1,(char*)msg.c_str()));
		}catch(socket_exception&){}
	}
	int get_message_num(){ return message.size(); }
	string get_message(){
		if (message.size()>0){
			string ret=message.front();
			message.pop();
			return ret;
		}
		else return string("");
	}

	// 内部処理
	void run(){
		// まずは接続
		if (server){
			server_socket *ss=new server_socket(port);
			s=ss->accept();
			delete ss; // サーバーソケットって閉じても大丈夫？
		}
		else{
			s=new socket_obj(host,port);
		}

		s->set_no_delay(true);

		// SRAM転送要求がきてたら送受信
		while(my_sram==NULL) thread::sleep(10);
		send_packet(packet("SRAM",my_sram_size,(char*)my_sram));

		packet p=recv_packet();
		if (p.tag!="SRAM"){ // どないしよ…
			delete s;
			s=NULL;
			return;
		}
		opp_sram=p.dat;
		opp_sram_size=p.size;

		// 遅延速度を計測しておく 20回ぐらいのメディアンでいいか
		{
			data_type dmy;
			if (server){
				int d[20];
				for (int i=0;i<20;i++){
					DWORD start=timeGetTime();
					send_packet(packet("TEST",sizeof(dmy),(char*)&dmy));
					packet p=recv_packet();
					d[i]=timeGetTime()-start;
					delete []p.dat;
				}
				sort(d,d+19);
				delay=d[9];
				send_packet(packet("DELY",4,(char*)&delay));
			}
			else{
				for (int i=0;i<20;i++){
					packet p=recv_packet();
					send_packet(packet("TEST",sizeof(dmy),(char*)&dmy));
					delete []p.dat;
				}
				packet p=recv_packet();
				delay=*(int*)p.dat;
				delete []p.dat;
			}
		}

		prepared=true;

		// パケット処理ループ
		while (alive){
			if (!s->connected())
				break;
			else if (s->ready_to_receive()){
				packet p;
				try{
					p=recv_packet();
				} catch(socket_exception&){
					break;
				}

				if (p.tag=="KEY "){
					if (p.size==sizeof(data_type))
						push_opp_key(*(data_type*)p.dat);
				}
				else if (p.tag=="MSG "){
					if (p.size==strlen(p.dat)+1) // ひとまず簡単なチェックを
						message.push(string(p.dat));
				}
				delete []p.dat;
			}
			else
				sleep(4); // 適度に待つ
		}

		delete s;
		s=NULL;
	}

private:
	// パケット処理周り
	class packet{
	public:
		packet(char *tag="",int size=0,char *dat=NULL){
			this->tag=string(tag);
			this->size=size;
			this->dat=dat;
		}
		string tag;
		int size;
		char *dat;
	};
	void send_packet(packet p){
		if (s==NULL) return; // とりあえず無視するということで。
		if (p.tag.length()!=4) return; // これも
		try{
			s->send((void*)p.tag.c_str(),4);
			s->send(&p.size,4);
			s->send(p.dat,p.size);
		}catch(socket_exception&){ /* 無視する */ }
	}
	packet recv_packet(){
		if (s==NULL) return packet();

		char *tag=new char[5];
		s->read(tag,4);
		tag[4]='\0';

		int size;
		s->read(&size,4);

		char *dat=new char[size];
		s->read(dat,size);
		packet p=packet(tag,size,dat);
		delete []tag;
		return p;
	}

	// SRAM周り
	volatile char *my_sram,*opp_sram;
	volatile int my_sram_size,opp_sram_size;

	// キーデータの格納
	queue<data_type> my_keydata,opp_keydata;
	void push_opp_key(data_type &dat){
		critical_lock cl(cs);
		opp_keydata.push(dat);
	}

	// チャットデータ格納
	queue<string> message;

	// 接続周り
	bool server;
	string host;
	int port;
	socket_obj *s;

	// ディレイ (in ms)
	volatile int delay;

	volatile bool prepared;
	bool alive;
	critical_section cs;
};
