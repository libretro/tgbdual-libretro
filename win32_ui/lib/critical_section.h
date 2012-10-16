#pragma once

#include <windows.h>

class critical_section{
public:
	critical_section(){
		InitializeCriticalSection(&cs);
	}
	~critical_section(){
		DeleteCriticalSection(&cs);
	}

	void enter(){
		EnterCriticalSection(&cs);
	}
	void leave(){
		LeaveCriticalSection(&cs);
	}

private:
	CRITICAL_SECTION cs;
};

class critical_lock{
public:
	critical_lock(critical_section &cs){
		this->cs=&cs;
		cs.enter();
	}
	~critical_lock(){
		cs->leave();
	}

private:
	critical_section *cs;
};
