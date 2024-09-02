
# VERBOSE can be set to 1 to echo all commands performed by this makefile 
ifeq ($(VERBOSE),1)
	VB=''
else
	VB=@
endif


SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS = `sdl2-config --cflags --libs` -lSDL2_mixer


CC = clang  # gcc or g++
CFLAGS += -DFEATURE_SOUND $(SDL_CFLAGS)
LDFLAGS +=
LIBS += -lm -lc $(SDL_LIBS)

# subdirectory for all produced objects/executables
OUTPUT_DIR = build
OUTPUT_DIR_SDL = $(OUTPUT_DIR)/sdl_specific
OUTPUT = $(OUTPUT_DIR)/doom

SRC_DOOM = dummy.o am_map.o doomdef.o doomstat.o dstrings.o d_event.o d_items.o d_iwad.o d_loop.o d_main.o d_mode.o d_net.o f_finale.o f_wipe.o g_game.o hu_lib.o hu_stuff.o info.o i_cdmus.o i_endoom.o i_joystick.o i_scale.o i_sound.o i_system.o i_timer.o memio.o m_argv.o m_bbox.o m_cheat.o m_config.o m_controls.o m_fixed.o m_menu.o m_misc.o m_random.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o r_segs.o r_sky.o r_things.o sha1.o sounds.o statdump.o st_lib.o st_stuff.o s_sound.o tables.o v_video.o wi_stuff.o w_checksum.o w_file.o w_main.o w_wad.o z_zone.o w_file_stdc.o i_input.o i_video.o doomgeneric.o
SRC_DOOM_SDL = doomgeneric_sdl.o mus2mid.o i_sdlmusic.o i_sdlsound.o

OBJS += $(addprefix $(OUTPUT_DIR)/, $(SRC_DOOM))
OBJS_SDL += $(addprefix $(OUTPUT_DIR_SDL)/, $(SRC_DOOM_SDL))

all: $(OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)

$(OUTPUT): $(OBJS) $(OBJS_SDL)
	@echo [Linking $@]
	$(VB)$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(OBJS_SDL) -o $@ $(LIBS)

$(OBJS): | $(OUTPUT_DIR)

$(OUTPUT_DIR):
	$(VB)mkdir -p $@

$(OBJS_SDL): | $(OUTPUT_DIR_SDL)

$(OUTPUT_DIR_SDL):
	$(VB)mkdir -p $@

$(OUTPUT_DIR)/%.o: ../src/%.c
	@echo [Compiling $<]
	$(VB)$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR_SDL)/%.o: %.c
	@echo [Compiling $<]
	$(VB)$(CC) $(CFLAGS) -I$(shell pwd)/../src -c $< -o $@

print:
	@echo OBJS: $(OBJS)
	@echo OBJS_SDL: $(OBJS_SDL)
