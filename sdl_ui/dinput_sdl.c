#include "dinput_sdl.h"

/* from SDL_dx5events.c */
void DX5_InitOSKeymap()
{
#ifndef DIK_PAUSE
#define DIK_PAUSE	0xC5
#endif
#ifndef DIK_OEM_102
#define DIK_OEM_102	0x56	/* < > | on UK/Germany keyboards */
#endif
	int i;

	/* Map the DIK scancodes to SDL keysyms */
	for ( i=0; i<SDL_TABLESIZE(DIK_keymap); ++i )
		DIK_keymap[i] = 0;

	/* Defined DIK_* constants */
	DIK_keymap[DIK_ESCAPE] = SDLK_ESCAPE;
	DIK_keymap[DIK_1] = SDLK_1;
	DIK_keymap[DIK_2] = SDLK_2;
	DIK_keymap[DIK_3] = SDLK_3;
	DIK_keymap[DIK_4] = SDLK_4;
	DIK_keymap[DIK_5] = SDLK_5;
	DIK_keymap[DIK_6] = SDLK_6;
	DIK_keymap[DIK_7] = SDLK_7;
	DIK_keymap[DIK_8] = SDLK_8;
	DIK_keymap[DIK_9] = SDLK_9;
	DIK_keymap[DIK_0] = SDLK_0;
	DIK_keymap[DIK_MINUS] = SDLK_MINUS;
	DIK_keymap[DIK_EQUALS] = SDLK_EQUALS;
	DIK_keymap[DIK_BACK] = SDLK_BACKSPACE;
	DIK_keymap[DIK_TAB] = SDLK_TAB;
	DIK_keymap[DIK_Q] = SDLK_q;
	DIK_keymap[DIK_W] = SDLK_w;
	DIK_keymap[DIK_E] = SDLK_e;
	DIK_keymap[DIK_R] = SDLK_r;
	DIK_keymap[DIK_T] = SDLK_t;
	DIK_keymap[DIK_Y] = SDLK_y;
	DIK_keymap[DIK_U] = SDLK_u;
	DIK_keymap[DIK_I] = SDLK_i;
	DIK_keymap[DIK_O] = SDLK_o;
	DIK_keymap[DIK_P] = SDLK_p;
	DIK_keymap[DIK_LBRACKET] = SDLK_LEFTBRACKET;
	DIK_keymap[DIK_RBRACKET] = SDLK_RIGHTBRACKET;
	DIK_keymap[DIK_RETURN] = SDLK_RETURN;
	DIK_keymap[DIK_LCONTROL] = SDLK_LCTRL;
	DIK_keymap[DIK_A] = SDLK_a;
	DIK_keymap[DIK_S] = SDLK_s;
	DIK_keymap[DIK_D] = SDLK_d;
	DIK_keymap[DIK_F] = SDLK_f;
	DIK_keymap[DIK_G] = SDLK_g;
	DIK_keymap[DIK_H] = SDLK_h;
	DIK_keymap[DIK_J] = SDLK_j;
	DIK_keymap[DIK_K] = SDLK_k;
	DIK_keymap[DIK_L] = SDLK_l;
	DIK_keymap[DIK_SEMICOLON] = SDLK_SEMICOLON;
	DIK_keymap[DIK_APOSTROPHE] = SDLK_QUOTE;
	DIK_keymap[DIK_GRAVE] = SDLK_BACKQUOTE;
	DIK_keymap[DIK_LSHIFT] = SDLK_LSHIFT;
	DIK_keymap[DIK_BACKSLASH] = SDLK_BACKSLASH;
	DIK_keymap[DIK_OEM_102] = SDLK_BACKSLASH;
	DIK_keymap[DIK_Z] = SDLK_z;
	DIK_keymap[DIK_X] = SDLK_x;
	DIK_keymap[DIK_C] = SDLK_c;
	DIK_keymap[DIK_V] = SDLK_v;
	DIK_keymap[DIK_B] = SDLK_b;
	DIK_keymap[DIK_N] = SDLK_n;
	DIK_keymap[DIK_M] = SDLK_m;
	DIK_keymap[DIK_COMMA] = SDLK_COMMA;
	DIK_keymap[DIK_PERIOD] = SDLK_PERIOD;
	DIK_keymap[DIK_SLASH] = SDLK_SLASH;
	DIK_keymap[DIK_RSHIFT] = SDLK_RSHIFT;
	DIK_keymap[DIK_MULTIPLY] = SDLK_KP_MULTIPLY;
	DIK_keymap[DIK_LMENU] = SDLK_LALT;
	DIK_keymap[DIK_SPACE] = SDLK_SPACE;
	DIK_keymap[DIK_CAPITAL] = SDLK_CAPSLOCK;
	DIK_keymap[DIK_F1] = SDLK_F1;
	DIK_keymap[DIK_F2] = SDLK_F2;
	DIK_keymap[DIK_F3] = SDLK_F3;
	DIK_keymap[DIK_F4] = SDLK_F4;
	DIK_keymap[DIK_F5] = SDLK_F5;
	DIK_keymap[DIK_F6] = SDLK_F6;
	DIK_keymap[DIK_F7] = SDLK_F7;
	DIK_keymap[DIK_F8] = SDLK_F8;
	DIK_keymap[DIK_F9] = SDLK_F9;
	DIK_keymap[DIK_F10] = SDLK_F10;
	DIK_keymap[DIK_NUMLOCK] = SDLK_NUMLOCK;
	DIK_keymap[DIK_SCROLL] = SDLK_SCROLLOCK;
	DIK_keymap[DIK_NUMPAD7] = SDLK_KP7;
	DIK_keymap[DIK_NUMPAD8] = SDLK_KP8;
	DIK_keymap[DIK_NUMPAD9] = SDLK_KP9;
	DIK_keymap[DIK_SUBTRACT] = SDLK_KP_MINUS;
	DIK_keymap[DIK_NUMPAD4] = SDLK_KP4;
	DIK_keymap[DIK_NUMPAD5] = SDLK_KP5;
	DIK_keymap[DIK_NUMPAD6] = SDLK_KP6;
	DIK_keymap[DIK_ADD] = SDLK_KP_PLUS;
	DIK_keymap[DIK_NUMPAD1] = SDLK_KP1;
	DIK_keymap[DIK_NUMPAD2] = SDLK_KP2;
	DIK_keymap[DIK_NUMPAD3] = SDLK_KP3;
	DIK_keymap[DIK_NUMPAD0] = SDLK_KP0;
	DIK_keymap[DIK_DECIMAL] = SDLK_KP_PERIOD;
	DIK_keymap[DIK_F11] = SDLK_F11;
	DIK_keymap[DIK_F12] = SDLK_F12;

	DIK_keymap[DIK_F13] = SDLK_F13;
	DIK_keymap[DIK_F14] = SDLK_F14;
	DIK_keymap[DIK_F15] = SDLK_F15;

	DIK_keymap[DIK_NUMPADEQUALS] = SDLK_KP_EQUALS;
	DIK_keymap[DIK_NUMPADENTER] = SDLK_KP_ENTER;
	DIK_keymap[DIK_RCONTROL] = SDLK_RCTRL;
	DIK_keymap[DIK_DIVIDE] = SDLK_KP_DIVIDE;
	DIK_keymap[DIK_SYSRQ] = SDLK_SYSREQ;
	DIK_keymap[DIK_RMENU] = SDLK_RALT;
	DIK_keymap[DIK_PAUSE] = SDLK_PAUSE;
	DIK_keymap[DIK_HOME] = SDLK_HOME;
	DIK_keymap[DIK_UP] = SDLK_UP;
	DIK_keymap[DIK_PRIOR] = SDLK_PAGEUP;
	DIK_keymap[DIK_LEFT] = SDLK_LEFT;
	DIK_keymap[DIK_RIGHT] = SDLK_RIGHT;
	DIK_keymap[DIK_END] = SDLK_END;
	DIK_keymap[DIK_DOWN] = SDLK_DOWN;
	DIK_keymap[DIK_NEXT] = SDLK_PAGEDOWN;
	DIK_keymap[DIK_INSERT] = SDLK_INSERT;
	DIK_keymap[DIK_DELETE] = SDLK_DELETE;
	DIK_keymap[DIK_LWIN] = SDLK_LMETA;
	DIK_keymap[DIK_RWIN] = SDLK_RMETA;
	DIK_keymap[DIK_APPS] = SDLK_MENU;
}
