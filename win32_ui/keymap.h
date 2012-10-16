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

//--------------------------------------------------
// キー名

static char keyboad_map[0x100][7]={
	"","ESC","1","2","3","4","5","6","7","8","9","0","-","","BS","TAB",
	"Q","W","E","R","T","Y","U","I","O","P","[","]","Enter","L-Ctrl","A","S",
	"D","F","G","H","J","K","L",";","","","L-Shft","_","Z","X","C","V",
	"B","N","M",",",".","/","R-Shft","Num-*","L-Alt","Space","英数","F1","F2","F3","F4","F5",
	"F6","F7","F8","F9","F10","Num-Lk","Scrl","Num-7","Num-8","Num-9","Num -","Num-4","Num-5","Num-6","Num +","Num-1",
	"Num-2","Num-3","Num-0","Num .","","","","F11","F12","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"かな","","","","","","","","","変換","","無変換","","\\","","",
	"","","","","","","","","","","","","","","","",
	"^","@",":","","漢字","","","","","","","","NuEnt","R-Ctrl","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","Num /","","PrScr","R-Alt","","","","","","","",
	"","","","","","","","Home","↑","Pg-up","","←","","→","","End",
	"↓","Pg-dn","INS","DEL","","","","","","","","L-Win","R-Win","R-Menu","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","",""
};

