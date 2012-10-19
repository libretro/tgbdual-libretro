all:
	$(MAKE) -C gb_core
	$(MAKE) -C libretro_ui
	cp libretro_ui/libretro.so .

clean:
	$(MAKE) -C gb_core clean
	$(MAKE) -C libretro_ui clean

