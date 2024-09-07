
# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif


PATH_TO_THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
DIR_CONTAINING_THIS_MAKEFILE := $(dir PATH_TO_THIS_MAKEFILE)


###########################################################################
# Targets for building a WebAssembly module from the Doom source code
###########################################################################

CFLAGS += --target=wasm32-unknown-wasi -Wall -g -Os
LDFLAGS +=
LIBS += -lm -lc

OUTPUT_DIR = build
OUTPUT_NAME = doom.wasm
OUTPUT = $(OUTPUT_DIR)/$(OUTPUT_NAME)

SRC_DOOM = dummy.o am_map.o doomdef.o doomstat.o dstrings.o d_event.o d_items.o d_iwad.o d_loop.o d_main.o d_mode.o d_net.o f_finale.o f_wipe.o g_game.o hu_lib.o hu_stuff.o info.o i_cdmus.o i_endoom.o i_joystick.o i_scale.o i_sound.o i_system.o i_timer.o memio.o m_argv.o m_bbox.o m_cheat.o m_config.o m_controls.o m_fixed.o m_menu.o m_misc.o m_random.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o r_segs.o r_sky.o r_things.o sha1.o sounds.o statdump.o st_lib.o st_stuff.o s_sound.o tables.o v_video.o wi_stuff.o w_checksum.o w_file.o w_wad.o z_zone.o w_file_stdc.o i_input.o i_video.o doomgeneric.o
OBJS += $(addprefix $(OUTPUT_DIR)/, $(SRC_DOOM))

# The WASI SDK (https://github.com/WebAssembly/wasi-sdk) conventiently packages up all that's needed to use Clang to compile to WebAssembly,
# and what's even better is that they provide a Docker image with all needed tools already installed and configured: https://github.com/WebAssembly/wasi-sdk?tab=readme-ov-file#docker-image
RUN_IN_WASI_SDK_DOCKER_IMAGE = docker run --rm -v $(DIR_CONTAINING_THIS_MAKEFILE):/repo -w /repo ghcr.io/webassembly/wasi-sdk:wasi-sdk-24


all: doom

clean:
	rm -rf $(OUTPUT_DIR)

# Provide a target that builds the main artifact by immediately delegating to the WASI SDK docker image to build everything needed.
# This target exists as a way to save time when the main artifact has to be built from scratch.
# It saves time by not having to step into and out of a docker container each time one dependency of the main artifact has to be built.
# One sampling of the time difference between this approach and the alternative `make $(OUTPUT)` showed a savings of around 30% (~2 mins vs. ~3 mins) when building from scratch
doom:
	@echo [Delegating to WASI SDK Docker image]
	$(VB)${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $(OUTPUT)

# Link all object files together to produce the needed artifact, but make sure that linking happens via the WASI SDK.
# We do this by detecting when the literal phrase 'wasi' DOESN'T appear in the path to the compiler
# (e.g. you're running locally and aren't configured to use a local WASI SDK installation)
# and then rerunning this `make` target in a docker container based on the WASI SDK docker image.
$(OUTPUT): $(OBJS)
	$(VB)if echo "$(CC)" | grep -q "wasi"; then \
		echo [Linking $@]; \
		$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -s -o $@ $(LIBS); \
	else \
		echo [Delegating to WASI SDK Docker image]; \
		${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $@; \
	fi

$(OBJS): | $(OUTPUT_DIR)

$(OUTPUT_DIR):
	@echo [Creating output folder $@]
	$(VB)mkdir -p $(OUTPUT_DIR)

# Produce object files by compiling the related source file, but make sure that compiling happens via the WASI SDK.
# We do this by detecting when the literal phrase 'wasi' DOESN'T appear in the path to the compiler
# (e.g. you're running locally and aren't configured to use a local WASI SDK installation)
# and then rerunning this `make` target in a docker container based on the WASI SDK docker image.
$(OUTPUT_DIR)/%.o:	src/%.c
	$(VB)if echo "$(CC)" | grep -q "wasi"; then \
		echo [Compiling $<]; \
		$(CC) $(CFLAGS) -I$(DIR_CONTAINING_THIS_MAKEFILE)/include -c $< -o $@; \
	else \
		echo [Delegating to WASI SDK Docker image]; \
		${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $@; \
	fi


##########################################################
# Targets for managing the local Python dev environment
##########################################################

DEV_PYTHON_VIRTUAL_ENV = .dev_virtualenv
# Dependencies in Python dev environment are managed via a Requirements file, stored as dev_requirements.txt
DEV_REQUIREMENTS_TXT = dev_requirements.txt

dev-init: | ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Installing pre-commit hooks]
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit install; \
	)

dev-clean:
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit uninstall; \
	)
	${VB} rm -fr ${DEV_PYTHON_VIRTUAL_ENV}

${DEV_PYTHON_VIRTUAL_ENV}:
	@echo [Creating Python virtual environment at '${DEV_PYTHON_VIRTUAL_ENV}/' for development needs]
	${VB}python3 -m venv ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		echo [Initialize dev Python virtual environment with all needed dependencies]; \
		python -m pip install -r ${DEV_REQUIREMENTS_TXT}; \
	)

# Generate/update dev requirements file to match the package versions being used in the Python dev environment
# (this is done via a .PHONY target because it's not clear how we could direct 'make' to determine when this
# file is truly out of date, so instead we just always assume this file is out of date)
generate-dev-requirements: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}> ${DEV_REQUIREMENTS_TXT}
	@echo "##################################################################################################" >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# This file is used to manage which Python modules are installed in the virtual environment that lives at ${DEV_PYTHON_VIRTUAL_ENV}/" >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# WARNING: This file was auto-generated, likely by calling something similar to 'make $@'." >> ${DEV_REQUIREMENTS_TXT}
	@echo "# Any manual edits made to this file will probably be clobbered!" >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# If you wish to change which Python modules are installed in the virtual environment at ${DEV_PYTHON_VIRTUAL_ENV}/" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# you should, instead, just manually install/uninstall modules in that virtual environment as you see fit" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# (see here if you're unsure of how to interact with Python virtual environment: https://docs.python.org/3/tutorial/venv.html)" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# and then manually run 'make $@'." >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "##################################################################################################" >> ${DEV_REQUIREMENTS_TXT}
	@echo "" >> ${DEV_REQUIREMENTS_TXT}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		python -m pip freeze >> ${DEV_REQUIREMENTS_TXT}; \
	)


##########################################################
# Targets for manually running pre-commit tests
##########################################################

run-precommit-on-all-files: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit run --all-files; \
	)

run-precommit-on-staged-files: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit run; \
	)


.PHONY: doom generate-dev-requirements run-precommit-on-all-files run-precommit-on-staged-files
