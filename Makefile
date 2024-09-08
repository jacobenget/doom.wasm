
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

# Produce a text file that describes the imports and exports of the Doom WebAssembly module.
$(OUTPUT_NAME).interface.txt: $(OUTPUT) utils/print-interface-of-wasm-module.mjs
	$(VB) > $@
	$(VB)echo "##############################################################################" >> $@
	$(VB)echo "#" >> $@
	$(VB)echo "# Below lies the interface to the WebAssembly module located at '$(OUTPUT)'" >> $@
	$(VB)echo "#" >> $@
	$(VB)echo "# WARNING: This file was auto-generated, likely by calling something similar to 'make $@'." >> $@
	$(VB)echo "# Any manual edits made to this file will probably be clobbered!" >> $@
	$(VB)echo "#" >> $@
	$(VB)echo "##############################################################################" >> $@
	$(VB)echo "" >> $@
	$(VB)docker run --rm -v $(DIR_CONTAINING_THIS_MAKEFILE):/repo -w /repo node:20.17.0 node utils/print-interface-of-wasm-module.mjs $(OUTPUT) >> $@


##########################################################
# Targets for managing the local dev environments
##########################################################

DEV_VIRTUAL_ENVS_DIR = .dev_virtualenvs

# A virtual Python environment is used in dev for managing pre-commit hooks, and to install the Rust dev environment
DEV_PYTHON_VIRTUAL_ENV = $(DEV_VIRTUAL_ENVS_DIR)/python_virtualenv
# A virtual Rust environment is used for some utilities
DEV_RUST_VIRTUAL_ENV = $(DEV_VIRTUAL_ENVS_DIR)/rust_virtualenv

ACTIVATE_DEV_PYTHON_VIRTUAL_ENV = source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate

# Dependencies in the Python dev environment are managed via a Requirements file, stored as python_dev_requirements.txt
PYTHON_DEV_REQUIREMENTS_TXT = $(DEV_VIRTUAL_ENVS_DIR)/python_dev_requirements.txt

# Each Rust utility should be listed here, in order to make sure it is properly configured when related make-targets are run
RUST_UTILS =
BUILD_RUST_UTILS = $(addprefix build-rust-util_, $(RUST_UTILS))
REMOVE_ACTIVATE_LINK_FROM_RUST_UTILS = $(addprefix remove-hard-link-to-rust-venv_, $(RUST_UTILS))

# Why build all of the Rust utils on dev-init? This is done to frontload all the work of building the dependencies of these utils.
dev-init: install-pre-commit-hooks $(BUILD_RUST_UTILS) | ${DEV_PYTHON_VIRTUAL_ENV} ${DEV_RUST_VIRTUAL_ENV}

dev-clean: uninstall-pre-commit-hooks $(REMOVE_ACTIVATE_LINK_FROM_RUST_UTILS)
	${VB} rm -fr ${DEV_PYTHON_VIRTUAL_ENV}
	${VB} rm -fr ${DEV_RUST_VIRTUAL_ENV}

install-pre-commit-hooks: | ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Installing pre-commit hooks]
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		pre-commit install; \
	)

uninstall-pre-commit-hooks: | ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Uninstalling pre-commit hooks]
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		pre-commit uninstall; \
	)

${DEV_PYTHON_VIRTUAL_ENV}:
	@echo [Creating Python virtual environment at '${DEV_PYTHON_VIRTUAL_ENV}/' for development needs]
	${VB}python3 -m venv ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		echo [Initialize dev Python virtual environment with all needed dependencies]; \
		python -m pip install -r ${PYTHON_DEV_REQUIREMENTS_TXT}; \
	)

# Update Python dev requirements file to match the package versions being used in the Python dev environment
# (this is done via a .PHONY target because it's not clear how we could direct 'make' to determine when this
# file is truly out of date, so instead we just always assume this file is out of date)
generate-python-dev-requirements: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "##################################################################################################" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# This file is used to manage which Python modules are installed in the virtual environment that lives at ${DEV_PYTHON_VIRTUAL_ENV}/" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# WARNING: This file was auto-generated, likely by calling something similar to 'make $@'." >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# Any manual edits made to this file will probably be clobbered!" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# If you wish to change which Python modules are installed in the virtual environment at ${DEV_PYTHON_VIRTUAL_ENV}/" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# you should, instead, just manually install/uninstall modules in that virtual environment as you see fit" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# (see here if you're unsure of how to interact with Python virtual environment: https://docs.python.org/3/tutorial/venv.html)" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "# and then manually run 'make $@'." >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "##################################################################################################" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	@echo "" >> ${PYTHON_DEV_REQUIREMENTS_TXT}
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		python -m pip freeze >> ${PYTHON_DEV_REQUIREMENTS_TXT}; \
	)

${DEV_RUST_VIRTUAL_ENV}: | ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Creating Rust virtual environment at '${DEV_RUST_VIRTUAL_ENV}/' for development needs]
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		rustenv $@; \
	)

# Each Rust utility will have, in its top-level directory, a hard link named `.rust_virtual_env_activate` that points
# to a script that may be 'sourced' in order to activate the dev Rust virtual environment.
HARD_LINK_TO_RUST_VIRTUAL_ENV_ACTIVATE_SCRIPT = .rust_virtual_env_activate

# Target for creating a hard link named `.rust_virtual_env_activate` that points to the activate script for the dev Rust virtual environment.
%/$(HARD_LINK_TO_RUST_VIRTUAL_ENV_ACTIVATE_SCRIPT): | ${DEV_RUST_VIRTUAL_ENV}
	@echo [Providing Rust virtual env activate script to the util '$*']
	${VB}ln ${DEV_RUST_VIRTUAL_ENV}/bin/activate $@;

# Target for building each of Rust utilities
$(BUILD_RUST_UTILS): build-rust-util_%: utils/%/$(HARD_LINK_TO_RUST_VIRTUAL_ENV_ACTIVATE_SCRIPT)
	@echo [Going to build the Rust util '$*']
	${VB}$(MAKE) --directory=utils/$*

# Target for removing, from each Rust util, the hard link pointing to the Rust virtual environment activate script.
$(REMOVE_ACTIVATE_LINK_FROM_RUST_UTILS): remove-hard-link-to-rust-venv_%:
	@echo [Removing hard link to Rust virtual env activate script that is in the util '$*']
	${VB}rm utils/$*/$(HARD_LINK_TO_RUST_VIRTUAL_ENV_ACTIVATE_SCRIPT)


##########################################################
# Targets for manually running pre-commit tests
##########################################################

run-precommit-on-all-files: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		pre-commit run --all-files; \
	)

run-precommit-on-staged-files: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		pre-commit run; \
	)


.PHONY: doom clean dev-init dev-clean install-pre-commit-hooks uninstall-pre-commit-hooks generate-python-dev-requirements run-precommit-on-all-files run-precommit-on-staged-files $(BUILD_RUST_UTILS) $(REMOVE_ACTIVATE_LINK_FROM_RUST_UTILS)
