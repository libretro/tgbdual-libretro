#include "libretro.h"
#include "../gb_core/gb.h"
#include "dmy_renderer.h"

static struct retro_system_info _info;
static struct retro_system_av_info _avinfo;

gb *g_gb[2];
dmy_renderer *render[2];

retro_video_refresh_t video_cb;
//retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
//retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;

void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name = "TGB Dual";
	info->library_version = "v0.8.3";
	info->need_fullpath = false;
	info->valid_extensions = "gb|gbc|sgb|GB|GBC|SGB";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing.fps = 60.0;
	info->timing.sample_rate = 44100.0;
	info->geometry.base_width = _avinfo.geometry.max_width = 160;
	info->geometry.base_height = _avinfo.geometry.max_height = 144;
	info->geometry.aspect_ratio = 10.0 / 9.0;
}

void retro_init(void)
{
	g_gb[0] = g_gb[1] = NULL;
	render[0] = render[1] = NULL;
}

void retro_deinit(void)
{
}


bool retro_load_game(const struct retro_game_info *info)
{
	render[0] = new dmy_renderer();
	g_gb[0] = new gb(render[0], true, true);
	g_gb[0]->load_rom((byte*)info->data, info->size, NULL, 0);
	return true;
}

void retro_unload_game(void)
{
	delete g_gb[0];
	delete render[0];
}

void retro_reset(void)
{
	g_gb[0]->reset();
}

void retro_run(void)
{
	render[0]->refresh();
	g_gb[0]->run();
}

void *retro_get_memory_data(unsigned id)
{
	switch(id) {
		case RETRO_MEMORY_SAVE_RAM:   return g_gb[0]->get_rom()->get_sram();
		case RETRO_MEMORY_RTC:        return &(render[0]->fixed_time);
		case RETRO_MEMORY_VIDEO_RAM:  return g_gb[0]->get_cpu()->get_vram();
		case RETRO_MEMORY_SYSTEM_RAM: return g_gb[0]->get_cpu()->get_ram();
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
		default: break;
	}
	return 0;
}



// TODO: savestates, cheats, load_game_special for 2 linked gb's
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data_, size_t size) { (void)data_; (void)size; return true; }
bool retro_unserialize(const void *data_, size_t size) { (void)data_; (void)size; return true; }

void retro_cheat_reset(void) { }
void retro_cheat_set(unsigned index, bool enabled, const char *code) { (void)index; (void)enabled; (void)code; }

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	(void)type;
	(void)info;
	(void)num;
	return false;
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
