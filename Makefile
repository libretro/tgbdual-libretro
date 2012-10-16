all:
	$(MAKE) -C gb_core
	$(MAKE) -C libretro_ui
	cp libretro_ui/libretro.so .

