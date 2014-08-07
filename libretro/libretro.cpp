#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1 // for fopencookie hack in serialize_size
#endif

#include <stdio.h>

#include "libretro.h"
#include "../gb_core/gb.h"
#include "dmy_renderer.h"

#define RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM ((3 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM ((4 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_GAME_TYPE_SUFAMI_TURBO 0x103

gb *g_gb[2];
dmy_renderer *render[2];

retro_log_printf_t log_cb;
retro_video_refresh_t video_cb;
//retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;

static size_t _serialize_size[2] = { 0, 0 };
extern bool _screen_2p_vertical;

#define _BOTH_GB_ for(int i=0; i<2; ++i) if(g_gb[i])

void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name = "TGB Dual";
	info->library_version = "v0.8.3";
	info->need_fullpath = false;
	info->valid_extensions = "gb|gbc|sgb|GB|GBC|SGB";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	int w = 160, h = 144;
	if (g_gb[1]) {
		// screen orientation for dual gameboy mode
		if(_screen_2p_vertical) {
			h *= 2;
		} else {
			w *= 2;
		}
	}
	info->timing.fps = 60.0f;
	info->timing.sample_rate = 44100.0f;
	info->geometry.base_width = info->geometry.max_width = w;
	info->geometry.base_height = info->geometry.max_height = h;
	info->geometry.aspect_ratio = float(w) / float(h);
}



void retro_init(void)
{
   struct retro_log_callback log;

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

	g_gb[0]   = g_gb[1]   = NULL;
	render[0] = render[1] = NULL;
}

void retro_deinit(void)
{
}



bool retro_load_game(const struct retro_game_info *info)
{
	render[0] = new dmy_renderer(0);
	g_gb[0] = new gb(render[0], true, true);
	g_gb[0]->load_rom((byte*)info->data, info->size, NULL, 0);
	_serialize_size[0] = _serialize_size[1] = 0;
	return true;
}

void retro_unload_game(void)
{
	_BOTH_GB_ {
		delete g_gb[i];   g_gb[i] = NULL;
		delete render[i]; render[i] = NULL;
	}
}

void retro_reset(void)
{
	_BOTH_GB_ g_gb[i]->reset();
}

void retro_run(void)
{
	input_poll_cb();
	for (int line=0;line<154;line++){
		if (g_gb[0])
			g_gb[0]->run();
		if (g_gb[1])
			g_gb[1]->run();
	}
}



void *retro_get_memory_data(unsigned id)
{
	switch(id) {
		case RETRO_MEMORY_SAVE_RAM:   return g_gb[0]->get_rom()->get_sram();
		case RETRO_MEMORY_RTC:        return &(render[0]->fixed_time);
		case RETRO_MEMORY_VIDEO_RAM:  return g_gb[0]->get_cpu()->get_vram();
		case RETRO_MEMORY_SYSTEM_RAM: return g_gb[0]->get_cpu()->get_ram();
		// for dual GB support.
		case RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM:
			if(g_gb[0]) return g_gb[0]->get_rom()->get_sram();
			break;
		case RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM:
			if(g_gb[1]) return g_gb[1]->get_rom()->get_sram();
			break;
		/* SGB was the initial attempt for dual-cart support.
		case RETRO_MEMORY_SNES_GAME_BOY_RAM:
			if(g_gb[1]) return g_gb[1]->get_rom()->get_sram();
			break;
		case RETRO_MEMORY_SNES_GAME_BOY_RTC:
			if(render[1]) return &(render[1]->fixed_time);
			break;*/
		default: break;
	}
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	switch(id) {
		case RETRO_MEMORY_SAVE_RAM: return g_gb[0]->get_rom()->get_sram_size();
		case RETRO_MEMORY_RTC:      return sizeof(render[0]->fixed_time);
		case RETRO_MEMORY_VIDEO_RAM:
			if (g_gb[0]->get_rom()->get_info()->gb_type >= 3) {
				return 0x2000*2; //sizeof(cpu::vram);
			}
			return 0x2000;
		case RETRO_MEMORY_SYSTEM_RAM:
			if (g_gb[0]->get_rom()->get_info()->gb_type >= 3) {
				return 0x2000*4; //sizeof(cpu::ram);
			}
			return 0x2000;
		// for dual GB support.
		case RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM:
			if(g_gb[0]) return g_gb[0]->get_rom()->get_sram_size();
			break;
		case RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM:
			if(g_gb[1]) return g_gb[1]->get_rom()->get_sram_size();
			break;
		/* SGB was the initial attempt for dual GB support.
		case RETRO_MEMORY_SNES_GAME_BOY_RAM:
			if(g_gb[1]) return g_gb[1]->get_rom()->get_sram_size();
			break;
		case RETRO_MEMORY_SNES_GAME_BOY_RTC:
			if(render[1]) return sizeof(render[1]->fixed_time);
			break; */
		default: break;
	}
	return 0;
}



// question: would saving both gb's into the same file be desirable ever?
// answer: yes, it's most likely needed to sync up netplay and for bsv records.
size_t retro_serialize_size(void)
{
	if ( ! (_serialize_size[0] + _serialize_size[1]) )
		_BOTH_GB_ _serialize_size[i] = g_gb[i]->get_state_size();
	return _serialize_size[0] + _serialize_size[1];
}

bool retro_serialize(void *data, size_t size)
{
	if (size == retro_serialize_size()) {
		uint8_t *ptr = (uint8_t*)data;
		_BOTH_GB_ {
			g_gb[i]->save_state_mem(ptr);
			ptr += _serialize_size[i];
		}
		return true;
	}
	return false;
}

bool retro_unserialize(const void *data, size_t size)
{
	if (size == retro_serialize_size()) {
		uint8_t *ptr = (uint8_t*)data;
		_BOTH_GB_ {
			g_gb[i]->restore_state_mem(ptr);
			ptr += _serialize_size[i];
		}
		return true;
	}
	return false;
}



bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	/* Originally used RETRO_GAME_TYPE_SUPER_GAME_BOY (num == 2), but then
	   both games don't have their SRAM loaded/saved by retroarch.
	   Would've been nice, for RTC support, but it doesn't save for the
	   base cartridge (g_gb[0] here), just the "actual" GB cartridge (g_gb[1]).
	 */
	if( !(type == RETRO_GAME_TYPE_SUFAMI_TURBO && num == 3) )
   {
      /*
      printf("Invalid load_game_special type: %x, %d\n", type, num);
      for (int i = 0; i < num; ++i) puts(info[i].path);
      puts("Load either a single normal GB/GBC game,");
      puts(" or load two GB/GBC games as slot A and B of 'Sufami Turbo'");
      puts(" (pick some dummy file for the BIOS, it won't be used)");
      */
      return false;
   }
	++info; // skip the "base cart"
	retro_load_game(&info[0]); // NB: this resets the _serialize_size array too
	render[1] = new dmy_renderer(1);
	g_gb[1] = new gb(render[1], true, true);
	g_gb[1]->load_rom((byte*)info[1].data, info[1].size, NULL, 0);
	// for link cables and IR:
	g_gb[0]->set_target(g_gb[1]);
	g_gb[1]->set_target(g_gb[0]);
	return true;
}



void retro_cheat_reset(void)
{
	_BOTH_GB_ g_gb[i]->get_cheat()->clear();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
#if 1==0
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "CHEAT:  id=%d, enabled=%d, code='%s'\n", index, enabled, code);
	// FIXME: work in progress.
	// As it stands, it seems TGB Dual only has support for Gameshark codes.
	// Unfortunately, the cheat.xml that ships with bsnes seems to only have
	// Game Genie codes, which are ROM patches rather than RAM.
	// http://nocash.emubase.de/pandocs.htm#gamegeniesharkcheats
	if(false && g_gb[0]) {
		cheat_dat cdat;
		cheat_dat *tmp=&cdat;

		strncpy(cdat.name, code, sizeof(cdat.name));

		tmp->enable = true;
		tmp->next = NULL;
		while(false) { // basically, iterate over code.split('+')
			// TODO: remove all non-alnum chars here
			if (false) { // if strlen is 9, game genie
				// game genie format: for "ABCDEFGHI",
				// AB   = New data
				// FCDE = Memory address, XORed by 0F000h
				// GIH  = Check data (can be ignored for our purposes)
				word scramble;
				sscanf(code, "%2hhx%4hx", &tmp->dat, &scramble);
				tmp->code = 1; // TODO: test if this is correct for ROM patching
				tmp->adr = (((scramble&0xF) << 12) ^ 0xF000) | (scramble >> 4);
			} else if (false) { // if strlen is 8, gameshark
				// gameshark format for "ABCDEFGH",
				// AB    External RAM bank number
				// CD    New Data
				// GHEF  Memory Address (internal or external RAM, A000-DFFF)
				byte adrlo, adrhi;
				sscanf(code, "%2hhx%2hhx%2hhx%2hhx", &tmp->code, &tmp->dat, &adrlo, &adrhi);
				tmp->adr = (adrhi<<8) | adrlo;
			}
			if(false) { // if there are more cheats left in the string
				tmp->next = new cheat_dat;
				tmp = tmp->next;
			}
		}
	}
	g_gb[0].get_cheat().add_cheat(&cdat);
#endif
}



// start boilerplate

unsigned retro_api_version(void) { return RETRO_API_VERSION; }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void retro_set_controller_port_device(unsigned port, unsigned device) { }

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { /*audio_cb = cb;*/ }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }
void retro_set_environment(retro_environment_t cb) { environ_cb = cb; }

// end boilerplate
