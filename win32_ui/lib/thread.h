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

#include <windows.h>
#include <process.h>

class runnable{
public:
	virtual ~runnable(){}
	virtual void run()=0;
};

// javaっぽいやつ。それよりはかなりしょぼいが。
// 使い方に注意。new thread(r)->start(); とかでスレッド作成。
// deleteするとスレッドが無条件で死ぬ。
class thread : public runnable{
public:
	thread(){
		r=this; // これでいのか？
	}
	thread(runnable *r){ // rの所有権も預ける
		this->r=r;
	}

	~thread(){
		if (thread_handle!=-1L){
			// とりあえずスレッドを道連れに
			TerminateThread((HANDLE)thread_handle,0);
		}
		if (r!=this) delete r;
	}

	void start(){
		if (r){
			thread_handle=_beginthread(invoke,0,(void*)this);
		}
	}

	bool running(){
		return thread_handle!=-1L;
	}

	void join(){
		if (running())
			WaitForSingleObject((HANDLE)thread_handle,INFINITE);
	}
	void join(int milisecond){
		if (running())
			WaitForSingleObject((HANDLE)thread_handle,milisecond);
	}

	static void sleep(int milisecond){
		::Sleep(milisecond);
	}
	static void yield(){
		::Sleep(0);
	}

	void run(){} // とりあえず空の実装

private:
	static void invoke(void *arg){
		thread *t=(thread*)arg;
		t->r->run();
		t->thread_handle=-1L;
	}

	runnable *r;
	uintptr_t thread_handle;
};
