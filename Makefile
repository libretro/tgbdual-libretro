all:
	$(MAKE) -C gb_core
	$(MAKE) -C libretro_ui

clean:
	$(MAKE) -C gb_core clean
	$(MAKE) -C libretro_ui clean

