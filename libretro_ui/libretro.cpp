#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1 // for fopencookie hack in serialize_size
#endif

#include <stdio.h>

#include "libretro.h"
#include "../gb_core/gb.h"
#include "dmy_renderer.h"

gb *g_gb[2];
dmy_renderer *render[2];

retro_video_refresh_t video_cb;
//retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
//retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;

static int _serialize_size = 0;

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
	if (g_gb[1]) h *= 2; // for dual gameboy mode
	info->timing.fps = 60.0f;
	info->timing.sample_rate = 44100.0f;
	info->geometry.base_width = info->geometry.max_width = w;
	info->geometry.base_height = info->geometry.max_height = h;
	info->geometry.aspect_ratio = float(w) / float(h);
}



void retro_init(void)
{
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
	_serialize_size = 0;
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
	_BOTH_GB_ g_gb[i]->run();
}



void *retro_get_memory_data(unsigned id)
{
	switch(id) {
		case RETRO_MEMORY_SAVE_RAM:   return g_gb[0]->get_rom()->get_sram();
		case RETRO_MEMORY_RTC:        return &(render[0]->fixed_time);
		case RETRO_MEMORY_VIDEO_RAM:  return g_gb[0]->get_cpu()->get_vram();
		case RETRO_MEMORY_SYSTEM_RAM: return g_gb[0]->get_cpu()->get_ram();
		case RETRO_MEMORY_SNES_GAME_BOY_RAM:
			if(g_gb[1]) return g_gb[1]->get_rom()->get_sram();
			break;
		case RETRO_MEMORY_SNES_GAME_BOY_RTC:
			if(render[1]) return &(render[1]->fixed_time);
			break;
		default: break;
	}
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	switch(id) {
		case RETRO_MEMORY_SAVE_RAM: return g_gb[0]->get_rom()->get_sram_size();
		case RETRO_MEMORY_RTC:      return sizeof(render[0]->fixed_time);
		case RETRO_MEMORY_VIDEO_RAM:  return 0x2000*2; //sizeof(cpu::vram);
		case RETRO_MEMORY_SYSTEM_RAM: return 0x2000*4; //sizeof(cpu::ram);
		case RETRO_MEMORY_SNES_GAME_BOY_RAM:
			if(g_gb[1]) return g_gb[1]->get_rom()->get_sram_size();
			break;
		case RETRO_MEMORY_SNES_GAME_BOY_RTC:
			if(render[1]) return sizeof(render[1]->fixed_time);
			break;
		default: break;
	}
	return 0;
}



// "counter" pseudo-file.
static ssize_t cookie_read(void *cookie, char *buf, size_t size)
{ *(int*)cookie += size; return size; }
static ssize_t cookie_write(void *cookie, const char *buf, size_t size)
{ *(int*)cookie += size; return size; }
static int cookie_seek(void *cookie, off64_t *offset, int whence)
{
	if (whence == SEEK_SET) { *(int*)cookie = *offset; }
	else if (whence == SEEK_CUR) { *(int*)cookie += *offset; }
	else { return -1; }
	return 0;
}
static int cookie_close(void *cookie) { return 0; }

static cookie_io_functions_t cookie_io = {
	.read = cookie_read,
	.write = cookie_write,
	.seek = cookie_seek,
	.close = cookie_close,
};

// question: would saving both gb's into the same file be desirable ever?
// answer: yes, it's most likely needed to sync up netplay and for bsv records.
size_t retro_serialize_size(void)
{
	if (!_serialize_size) {
		FILE* fcookie = fopencookie(&_serialize_size, "wb", cookie_io);
		_BOTH_GB_ g_gb[i]->save_state(fcookie);
		fclose(fcookie);
	}
	return _serialize_size;
}

bool retro_serialize(void *data_, size_t size)
{
	FILE *fmem = fmemopen(data_, size, "wb");
	_BOTH_GB_ g_gb[i]->save_state(fmem);
	return !fclose(fmem);
}

bool retro_unserialize(const void *data_, size_t size)
{
	FILE *fmem = fmemopen((void*)data_, size, "rb");
	_BOTH_GB_ g_gb[i]->restore_state(fmem);
	return !fclose(fmem);
}



bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	if( !(type == RETRO_GAME_TYPE_SUPER_GAME_BOY && num == 2) ) {
		return false;
	}
	retro_load_game(&info[0]);
	render[1] = new dmy_renderer(1);
	g_gb[1] = new gb(render[1], true, true);
	g_gb[1]->load_rom((byte*)info[1].data, info[1].size, NULL, 0);
	// TODO: figure out how to hook up ext ports and IR LEDs
	/* 
	struct ext_hook {
		byte (*send)(byte);
		bool (*led)(void);
	};

	g_gb[0].hook_extport( ? )
	cpu::seri_send ?
	 */
	return true;
}



void retro_cheat_reset(void)
{
	_BOTH_GB_ g_gb[i]->get_cheat()->clear();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	(void)index; (void)enabled; (void)code;
	// TODO
}



// start boilerplate

unsigned retro_api_version(void) { return RETRO_API_VERSION; }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void retro_set_controller_port_device(unsigned port, unsigned device) { }

void retro_set_environment(retro_environment_t cb) { /*environ_cb = cb;*/ }
void retro_set_audio_sample(retro_audio_sample_t cb) { /*audio_cb = cb;*/ }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }

// end boilerplate
