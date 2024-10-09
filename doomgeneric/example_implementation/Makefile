
# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif

CC = clang  # gcc or g++
CFLAGS += -DFEATURE_SOUND -I$(shell pwd)/.. -I$(shell pwd)/../include
LDFLAGS +=
LIBS += -lm

# subdirectory for all produced objects/executables
OUTPUT_DIR = build
OUTPUT_DIR_SDL_SPECIFIC = $(OUTPUT_DIR)/sdl_specific
OUTPUT_EXE_NAME = doom
OUTPUT = $(OUTPUT_DIR)/${OUTPUT_EXE_NAME}


SRC_DOOM = dummy.c am_map.c doomdef.c doomstat.c dstrings.c d_event.c d_items.c d_iwad.c d_loop.c d_main.c d_mode.c d_net.c f_finale.c f_wipe.c g_game.c hu_lib.c hu_stuff.c info.c i_cdmus.c i_endoom.c i_joystick.c i_scale.c i_sound.c i_system.c i_timer.c memio.c m_argv.c m_bbox.c m_cheat.c m_config.c m_controls.c m_fixed.c m_menu.c m_misc.c m_random.c p_ceilng.c p_doors.c p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c p_tick.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c r_segs.c r_sky.c r_things.c sha1.c sounds.c statdump.c st_lib.c st_stuff.c s_sound.c tables.c v_video.c wi_stuff.c w_checksum.c w_file.c w_wad.c z_zone.c i_input.c i_video.c doomgeneric.c
SRC_DOOM_SDL_SPECIFIC = doomgeneric_sdl.c mus2mid.c i_sdlmusic.c i_sdlsound.c file_misc.c

OBJS += $(addprefix $(OUTPUT_DIR)/, $(patsubst %.c, %.o, $(SRC_DOOM)))
OBJS_SDL += $(addprefix $(OUTPUT_DIR_SDL_SPECIFIC)/, $(patsubst %.c, %.o, $(SRC_DOOM_SDL_SPECIFIC)))


all: $(OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)

print:
	@echo OBJS: $(OBJS)
	@echo OBJS_SDL: $(OBJS_SDL)


#######################################
# Local instalation of SDL libraries
#######################################

# We'll depend upon a specific version of SDL and SDL_mixer that's been built and installed locally,
# to maximize the robustness of the SDL version of Doom we're using here as a testbed for changes.
INSTALL_DIR = installation
SDL_DIR = .SDL
SDL_MIXER_DIR = .SDL_mixer
SDL_INSTALL = $(SDL_DIR)/$(OUTPUT_DIR)/$(INSTALL_DIR)
SDL_MIXER_INSTALL = $(SDL_MIXER_DIR)/$(OUTPUT_DIR)/$(INSTALL_DIR)

# Retreive, build, and install a version of SDL that has been found to work for the needs of this project
$(SDL_INSTALL):
	git clone git@github.com:libsdl-org/SDL.git --branch release-2.30.7 --depth 1 $(SDL_DIR); \
	cd $(SDL_DIR); \
	mkdir -p $(OUTPUT_DIR)/$(INSTALL_DIR); \
	cd $(OUTPUT_DIR); \
	CC=$(CC) ../configure --prefix=$(shell pwd)/$(SDL_INSTALL); \
	make; \
	make install;

# Retreive, build, and install a version of SDL_mixer that has been found to work for the needs of this project
$(SDL_MIXER_INSTALL): | $(SDL_INSTALL)
	git clone git@github.com:libsdl-org/SDL_mixer.git --branch release-2.0.4 --depth 1 $(SDL_MIXER_DIR); \
	cd $(SDL_MIXER_DIR); \
	mkdir -p $(OUTPUT_DIR)/$(INSTALL_DIR); \
	cd $(OUTPUT_DIR); \
	CC=$(CC) ../configure --prefix=$(shell pwd)/$(SDL_MIXER_INSTALL) --with-sdl-prefix=$(shell pwd)/$(SDL_INSTALL); \
	make; \
	make install;

SDL_CFLAGS = -I$(SDL_INSTALL)/include/SDL2 -I$(SDL_MIXER_INSTALL)/include/SDL2 -D_THREAD_SAFE
SDL_LIBS = -L$(SDL_INSTALL)/lib -L$(SDL_MIXER_INSTALL)/lib -lSDL2 -lSDL2_mixer


#######################################
# Building of SDL Doom
#######################################

$(OUTPUT): $(OBJS) $(OBJS_SDL) | $(SDL_INSTALL) $(SDL_MIXER_INSTALL)
	@echo [Linking $@]
	$(VB)$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LDFLAGS) $(OBJS) $(OBJS_SDL) -o $@ $(LIBS) $(SDL_LIBS)

$(OBJS): | $(OUTPUT_DIR)

$(OUTPUT_DIR):
	@echo [Creating output folder $@]
	$(VB)mkdir -p $@

$(OBJS_SDL): | $(OUTPUT_DIR_SDL_SPECIFIC)

$(OUTPUT_DIR_SDL_SPECIFIC):
	@echo [Creating output folder $@]
	$(VB)mkdir -p $@

$(OUTPUT_DIR)/%.o: ../src/%.c | $(SDL_INSTALL) $(SDL_MIXER_INSTALL)
	@echo [Compiling $<]
	$(VB)$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(OUTPUT_DIR_SDL_SPECIFIC)/%.o: %.c | $(SDL_INSTALL) $(SDL_MIXER_INSTALL)
	@echo [Compiling $<]
	$(VB)$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@


#######################################
# Running of SDL Doom
#######################################

# Retrieve the Shareware Doom WAD, via the URL advertised here: https://doomwiki.org/wiki/DOOM1.WAD
$(OUTPUT_DIR)/DOOM1.WAD: | $(OUTPUT_DIR)
	@echo [Retreiving Shareware Doom WAD from the internet]
	$(VB)curl --output $(OUTPUT_DIR)/DOOM1.WAD https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad

# Note that Doom is going to use the current working directory to hold its configuration data (e.g. .savegame/),
# so when running Doom we make sure that directory is a directory not managed by source control.
run: $(OUTPUT_DIR)/DOOM1.WAD $(OUTPUT)
	@echo [Running SDL Doom with Shareware WAD]
	cd $(OUTPUT_DIR); ./${OUTPUT_EXE_NAME} -iwad ../$<

# Provide a way to run Doom in development mode.
# Development mode is useful because taking a screenshot by pressing F1 is supported.
#
# Note that Doom is going to use the current working directory to hold its configuration data (e.g. .savegame/),
# so when running Doom we make sure that directory is a directory not managed by source control.
run-dev-mode: $(OUTPUT_DIR)/DOOM1.WAD $(OUTPUT)
	@echo [Running SDL Doom with Shareware WAD, in development mode]
	cd $(OUTPUT_DIR); ./${OUTPUT_EXE_NAME} -iwad ../$< -devparm

# Note that Doom is going to use the current working directory to hold its configuration data (e.g. .savegame/),
# so when running Doom we make sure that directory is a directory not managed by source control.
run-with-test-pwad: $(OUTPUT)
	@echo [Running SDL Doom with GOTHIC2 WAD on top of Freedoom2]
	cd $(OUTPUT_DIR); ./${OUTPUT_EXE_NAME} -iwad ../test_wad_files/freedoom2.wad -skill 3 -file ../test_wad_files/GOTHIC2.WAD


.PHONY: clean run print
