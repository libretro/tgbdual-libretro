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

#ifndef w32_sdl_h_
#define w32_sdl_h_

#include <SDL.h>

// Win32 => SDL

typedef Uint32 DWORD;

inline Uint32 timeGetTime() {
	return SDL_GetTicks();
}

inline void Sleep(Uint32 t) {
	SDL_Delay(t);
}

#endif // ! w32_sdl_h_

