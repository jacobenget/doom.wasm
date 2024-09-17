
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

SRC_DOOM = dummy.o am_map.o doomdef.o doomstat.o dstrings.o d_event.o d_items.o d_iwad.o d_loop.o d_main.o d_mode.o d_net.o f_finale.o f_wipe.o g_game.o hu_lib.o hu_stuff.o info.o i_cdmus.o i_endoom.o i_joystick.o i_scale.o i_sound.o i_system.o i_timer.o memio.o m_argv.o m_bbox.o m_cheat.o m_config.o m_controls.o m_fixed.o m_menu.o m_misc.o m_random.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o r_segs.o r_sky.o r_things.o sha1.o sounds.o statdump.o st_lib.o st_stuff.o s_sound.o tables.o v_video.o wi_stuff.o w_checksum.o w_file.o w_wad.o z_zone.o i_input.o i_video.o doomgeneric.o
SRC_DOOM_SDL_SPECIFIC = doomgeneric_sdl.o mus2mid.o i_sdlmusic.o i_sdlsound.o file_misc.o

OBJS += $(addprefix $(OUTPUT_DIR)/, $(SRC_DOOM))
OBJS_SDL += $(addprefix $(OUTPUT_DIR_SDL_SPECIFIC)/, $(SRC_DOOM_SDL_SPECIFIC))


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
