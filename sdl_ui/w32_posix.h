/**
 * $Id$
 *
 * Copyright (C) shinichiro.h <s31552@mail.ecc.u-tokyo.ac.jp>
 *  http://user.ecc.u-tokyo.ac.jp/~s31552/wp/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef w32_posix_h_
#define w32_posix_h_

// Win32 => POSIX

#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

inline void GetCurrentDirectory(int len, char* dir) {
	getcwd(dir, len);
}

inline int SetCurrentDirectory(const char* dir) {
	return chdir(dir);
}

inline void CreateDirectory(const char* dir, void*) {

// 愛用しているマクロの引用文 
// I'd prefer to now treat cygwin much more unix like, and save the
// declspec() for
#if defined(WIN32) && !(defined(__CYGWIN__) || defined(__MINGW__))
// that way MSVC gets the declspec nastiness, and cygwin and mingw are
// spared. OR possibly
// #if defined(WIN32) && !defined(__CYGWIN__)
	mkdir(dir);
#else
	mkdir(dir, 0x1ff);
#endif  
}

typedef struct tm SYSTEMTIME;

inline void GetSystemTime(SYSTEMTIME* sys) {
	time_t t = time(0);
#if defined(WIN32) && !(defined(__CYGWIN__) || defined(__MINGW__))
	localtime(&t);
#else
	localtime_r(&t, sys);
#endif
}

#endif // ! w32_posix_h_

