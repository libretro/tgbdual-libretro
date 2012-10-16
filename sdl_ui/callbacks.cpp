#include "callbacks.h"

#include "w32_posix.h"
#include "setting.h"
#include "sdl_renderer.h"

#include "../gb_core/gb.h"

#include <SDL.h>

extern gb* g_gb[2];
extern setting *config;
extern Uint8* key_state;
extern sdl_renderer *render[2];
extern char tmp_sram_name[2][256];
extern bool b_running;

void cb_save_state(int lParam) {
	if (g_gb[0]){
		char cur_di[256],sv_dir[256];
		GetCurrentDirectory(256,cur_di);
		config->get_save_dir(sv_dir);
		SetCurrentDirectory(sv_dir);

		char name[256],*p;
		strcpy(name,tmp_sram_name[0]);
		if (!(p=strstr(name,".sav")))
			if (!(p=strstr(name,".ram")))
				return;
		int sav_slot;

		if ((int)lParam==-1)
			sav_slot=((key_state['1'])?1:((key_state['2'])?2:((key_state['3'])?3:((key_state['4'])?4:((key_state['5'])?5:((key_state['6'])?6:((key_state['7'])?7:((key_state['8'])?8:((key_state['9'])?9:0)))))))));
		else
			sav_slot=(int)lParam;

		sprintf(p,".sv%d",sav_slot);

		printf("save state %s\n", name);

		FILE *file=fopen(name,"wb");
		g_gb[0]->save_state(file);
		int tmp=render[0]->get_timer_state();
		fseek(file,-100,SEEK_CUR);
		fwrite(&tmp,4,1,file);
		fclose(file);

		char mes[32];
		sprintf(mes,"save state at %d        ",sav_slot);
		render[0]->show_message(mes);

		SetCurrentDirectory(cur_di);
	}
}

void cb_load_state(int lParam) {
	if (g_gb[0]){
		char cur_di[256],sv_dir[256];
		char mes[32];
		GetCurrentDirectory(256,cur_di);
		config->get_save_dir(sv_dir);
		SetCurrentDirectory(sv_dir);

		char name[256],*p;
		strcpy(name,tmp_sram_name[0]);
		if (!(p=strstr(name,".sav")))
			if (!(p=strstr(name,".ram")))
				return;

		int sav_slot;
		if ((int)lParam==-1)
			sav_slot=((key_state['1'])?1:((key_state['2'])?2:((key_state['3'])?3:((key_state['4'])?4:((key_state['5'])?5:((key_state['6'])?6:((key_state['7'])?7:((key_state['8'])?8:((key_state['9'])?9:0)))))))));
		else
			sav_slot=(int)lParam;

		sprintf(p,".sv%d",sav_slot);

		printf("load state %s\n", name);

		FILE *file=fopen(name,"rb");

		if (file){
			g_gb[0]->restore_state(file);
			int tmp;
			fseek(file,-100,SEEK_CUR);
			fread(&tmp,4,1,file);
			render[0]->set_timer_state(tmp);
			fclose(file);
			sprintf(mes,"restore state at %d          ",sav_slot);
			render[0]->show_message(mes);
		}
		else{
			sprintf(mes,"can't open state at %d         ",sav_slot);
			render[0]->show_message(mes);
		}

		SetCurrentDirectory(cur_di);
	}
}

void cb_pause(int) {
	printf("pause\n");
	if (g_gb[0]){
		b_running=!b_running;
		if (render[0])
			render[0]->resume_sound();
		if (!b_running)
			if (render[0])
				render[0]->pause_sound();
	}
}

void cb_fullscreen(int) {
	render[0]->pause_sound();
	render[0]->swith_screen_mode();
	render[0]->resume_sound();
	if (g_gb[0])
		g_gb[0]->refresh_pal();
}

extern bool b_terminal;

void cb_reset(int) {
	if (b_terminal) return;
	if (g_gb[0]) { g_gb[0]->set_use_gba(config->gb_type==0?config->use_gba:(config->gb_type==4?true:false));g_gb[0]->reset(); }
	if (render[0]) render[0]->reset();
}

extern bool end;

void cb_quit(int) {
	end = true;
}
