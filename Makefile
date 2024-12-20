
# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif


PATH_TO_THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
DIR_CONTAINING_THIS_MAKEFILE := $(dir PATH_TO_THIS_MAKEFILE)


all: doom


####################################################################
# Targets for managing the local dev environments and local tools
####################################################################

DEV_VIRTUAL_ENVS_DIR = .dev_virtualenvs

# A virtual Python environment is used in dev for managing pre-commit hooks, and to install the Rust dev environment
DEV_PYTHON_VIRTUAL_ENV = $(DEV_VIRTUAL_ENVS_DIR)/python_virtualenv
# A virtual Rust environment is used for some utilities
DEV_RUST_VIRTUAL_ENV = $(DEV_VIRTUAL_ENVS_DIR)/rust_virtualenv

ACTIVATE_DEV_PYTHON_VIRTUAL_ENV = source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate

# Dependencies in the Python dev environment are managed via a Requirements file, stored as python_dev_requirements.txt
PYTHON_DEV_REQUIREMENTS_TXT = $(DEV_VIRTUAL_ENVS_DIR)/python_dev_requirements.txt

# Each Rust utility should be listed here, in order to make sure it is properly configured when related make-targets are run
RUST_UTILS = print-interface-of-wasm-module
BUILD_RUST_UTILS = $(addprefix build-rust-util_, $(RUST_UTILS))
REMOVE_ACTIVATE_LINK_FROM_RUST_UTILS = $(addprefix remove-hard-link-to-rust-venv_, $(RUST_UTILS))

# Each Binaryen utility needed should be listed here so it can be built during dev-init
BINARYEN_DIR = .binaryen
BINARYEN_UTIL_NAMES = wasm-as wasm-merge wasm-metadce
BINARYEN_UTILS = $(addprefix $(BINARYEN_DIR)/bin/, $(BINARYEN_UTIL_NAMES))

WASM_AS = $(BINARYEN_DIR)/bin/wasm-as
WASM_MERGE = $(BINARYEN_DIR)/bin/wasm-merge
WASM_METADCE = $(BINARYEN_DIR)/bin/wasm-metadce

# Why build all of the Rust utils and Binaryen tools on dev-init? This is done to frontload all the work of building the dependencies of these utils.
dev-init: install-pre-commit-hooks $(BUILD_RUST_UTILS) $(BINARYEN_UTILS) | ${DEV_PYTHON_VIRTUAL_ENV} ${DEV_RUST_VIRTUAL_ENV}

dev-clean: uninstall-pre-commit-hooks $(REMOVE_ACTIVATE_LINK_FROM_RUST_UTILS)
	${VB} rm -fr ${DEV_PYTHON_VIRTUAL_ENV}
	${VB} rm -fr ${DEV_RUST_VIRTUAL_ENV}
	${VB} rm -fr ${BINARYEN_DIR}

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

${BINARYEN_DIR}:
	@echo [Configuring a local copy of Binaryen tools source code]
	${VB}git clone git@github.com:WebAssembly/binaryen.git --branch version_119 --depth 1 $@; \
	cd $@; \
	cmake -DBUILD_TESTS=OFF .;

# Target for building any Binaryen utility listed in BINARYEN_UTILS
#
# Note: A binaryen tool can take a long time to build, like 10 minutes or more, and these tools could just be run
# inside a docker container that has the tools pre-built, making it much quicker to build the main artifact of this
# repo from scratch.
# TODO: leverage Binaryen tools pre-built in an external docker image
$(BINARYEN_UTILS): ${BINARYEN_DIR}/bin/%: ${BINARYEN_DIR}
	@echo [Building the Binaryen util '$*']
	${VB}$(MAKE) --directory=$< $*


####################################################################################
# Targets for producing the main artifact of this repo: a Doom WebAssembly module 
####################################################################################

CFLAGS += --target=wasm32-unknown-wasi -Wall -g -Os
# Details on a few of the linker flags used:
#
#   -Wl,  <-- needed to pass the immediately following option directly to the linker,
#   --export-dynamic  <-- have linker export all non-hidden symbols
#       see https://lld.llvm.org/WebAssembly.html#cmdoption-export-dynamic
#   --import-undefined  <-- produce a WebAssembly import for any undefined symbols, where possible
#       see https://lld.llvm.org/WebAssembly.html#cmdoption-import-undefined
#   -mexec-model=reactor  <-- says "this isn't a command/program with a 'main' function that drives its execution, instead this is a library with a lifetime controlled by the user",
#       see https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-mexec-model
LDFLAGS += -Wl,--export-dynamic -Wl,--import-undefined -mexec-model=reactor
LIBS += -lm -lc

OUTPUT_DIR = build
OUTPUT_DIR_WASM_SPECIFIC = $(OUTPUT_DIR)/wasm_specific
OUTPUT_NAME = doom.wasm
OUTPUT = $(OUTPUT_DIR)/$(OUTPUT_NAME)
OUTPUT_RAW_FROM_LINKING = $(OUTPUT_DIR)/doom-directly-after-linking.wasm
OUTPUT_UTIL_WASM_MODULES = $(OUTPUT_DIR)/util-wasm-modules

FILE_EMBEDDED_IN_CODE_DIR = $(OUTPUT_DIR)/file_embedded_in_code
FILE_EMBEDDED_IN_CODE_OUTPUT_DIR = $(FILE_EMBEDDED_IN_CODE_DIR)/$(OUTPUT_DIR)

SRC_DOOM = dummy.c am_map.c doomdef.c doomstat.c dstrings.c d_event.c d_items.c d_iwad.c d_loop.c d_main.c d_mode.c d_net.c f_finale.c f_wipe.c g_game.c hu_lib.c hu_stuff.c info.c i_cdmus.c i_endoom.c i_joystick.c i_scale.c i_sound.c i_system.c i_timer.c memio.c m_argv.c m_bbox.c m_cheat.c m_config.c m_controls.c m_fixed.c m_menu.c m_misc.c m_random.c p_ceilng.c p_doors.c p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c p_tick.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c r_segs.c r_sky.c r_things.c sha1.c sounds.c statdump.c st_lib.c st_stuff.c s_sound.c tables.c v_video.c wi_stuff.c w_checksum.c w_file.c w_wad.c z_zone.c i_input.c i_video.c doomgeneric.c
SRC_DOOM_WASM_SPECIFIC = doom_wasm.c internal__wasi-snapshot-preview1.c
EMBEDDED_BINARY_FILES = DOOM1.WAD
SRC_FOR_EMBEDDED_FILES = $(addprefix $(FILE_EMBEDDED_IN_CODE_DIR)/, $(addsuffix .c, $(EMBEDDED_BINARY_FILES)))
HEADERS_FOR_EMBEDDED_FILES = $(addprefix $(FILE_EMBEDDED_IN_CODE_DIR)/, $(addsuffix .h, $(EMBEDDED_BINARY_FILES)))

OBJS += $(addprefix $(OUTPUT_DIR)/, $(patsubst %.c, %.o, $(SRC_DOOM)))
OBJS_WASM_SPECIFIC += $(addprefix $(OUTPUT_DIR_WASM_SPECIFIC)/, $(patsubst %.c, %.o, $(SRC_DOOM_WASM_SPECIFIC)))
OBJS_EMBEDDED_FILE += $(addprefix $(FILE_EMBEDDED_IN_CODE_OUTPUT_DIR)/, $(addsuffix .o, $(EMBEDDED_BINARY_FILES)))

# The WASI SDK (https://github.com/WebAssembly/wasi-sdk) conventiently packages up all that's needed to use Clang to compile to WebAssembly,
# and what's even better is that they provide a Docker image with all needed tools already installed and configured: https://github.com/WebAssembly/wasi-sdk?tab=readme-ov-file#docker-image
RUN_IN_WASI_SDK_DOCKER_IMAGE = docker run --rm -v $(DIR_CONTAINING_THIS_MAKEFILE):/repo -w /repo ghcr.io/webassembly/wasi-sdk:wasi-sdk-24


clean:
	rm -rf $(OUTPUT_DIR)

# TARGET THAT ONLY EXISTS AS AN OPTIMIZATION
#
# THis is a target that builds the main artifact by immediately delegating to the WASI SDK docker image to compile all source code.
# This target exists as a way to save time when the main artifact has to be built from scratch.
# It saves time by not having to step into and out of a docker container each time one dependency of the main artifact has to be built.
# One sampling of the time difference between this approach and the alternative `make $(OUTPUT)` showed a savings of around 30% (~2 mins vs. ~3 mins) when building from scratch.
doom: $(SRC_FOR_EMBEDDED_FILES) $(HEADERS_FOR_EMBEDDED_FILES)
	@echo [Delegating to WASI SDK Docker image just once]
	$(VB)${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $(OUTPUT_RAW_FROM_LINKING)
	@echo [Stepping out of WASI SDK Docker image for final steps of building the main artifact]
	$(VB)$(MAKE) $(MAKEFLAGS) $(OUTPUT)

# Link all object files together to produce the needed artifact, but make sure that linking happens via the WASI SDK.
# We do this by detecting when the literal phrase 'wasi' DOESN'T appear in the path to the compiler
# (e.g. you're running locally and aren't configured to use a local WASI SDK installation)
# and then rerunning this `make` target in a docker container based on the WASI SDK docker image.
$(OUTPUT_RAW_FROM_LINKING): $(OBJS) $(OBJS_WASM_SPECIFIC) $(OBJS_EMBEDDED_FILE)
	$(VB)if echo "$(CC)" | grep -q "wasi"; then \
		echo [Linking $@]; \
		$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(OBJS_WASM_SPECIFIC) $(OBJS_EMBEDDED_FILE) -s -o $@ $(LIBS); \
	else \
		echo [Delegating to WASI SDK Docker image]; \
		${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $@; \
	fi

$(OBJS): | $(OUTPUT_DIR)

$(OBJS_WASM_SPECIFIC): | $(OUTPUT_DIR_WASM_SPECIFIC)

$(OBJS_EMBEDDED_FILE): | $(FILE_EMBEDDED_IN_CODE_OUTPUT_DIR)

OUTPUT_DIRS = $(OUTPUT_DIR) $(OUTPUT_DIR_WASM_SPECIFIC) $(OUTPUT_UTIL_WASM_MODULES) $(FILE_EMBEDDED_IN_CODE_DIR) $(FILE_EMBEDDED_IN_CODE_OUTPUT_DIR)

$(OUTPUT_DIRS):
	@echo [Creating output folder \'$@\']
	$(VB)mkdir -p $@

# Produce object files by compiling the related source file, but make sure that compiling happens via the WASI SDK.
# We do this by detecting when the literal phrase 'wasi' DOESN'T appear in the path to the compiler
# (e.g. you're running locally and aren't configured to use a local WASI SDK installation)
# and then rerunning this `make` target in a docker container based on the WASI SDK docker image.
$(OUTPUT_DIR)/%.o:	doomgeneric/src/%.c
	$(VB)if echo "$(CC)" | grep -q "wasi"; then \
		echo [Compiling $<]; \
		$(CC) $(CFLAGS) -I$(DIR_CONTAINING_THIS_MAKEFILE)/doomgeneric -I$(DIR_CONTAINING_THIS_MAKEFILE)/doomgeneric/include -c $< -o $@; \
	else \
		echo [Delegating to WASI SDK Docker image]; \
		${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $@; \
	fi

$(OUTPUT_DIR_WASM_SPECIFIC)/%.o:	src/%.c $(HEADERS_FOR_EMBEDDED_FILES)
	$(VB)if echo "$(CC)" | grep -q "wasi"; then \
		echo [Compiling $<]; \
		$(CC) $(CFLAGS) -I$(DIR_CONTAINING_THIS_MAKEFILE)/doomgeneric -I$(OUTPUT_DIR) -c $< -o $@; \
	else \
		echo [Delegating to WASI SDK Docker image]; \
		${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $@; \
	fi

$(FILE_EMBEDDED_IN_CODE_OUTPUT_DIR)/%.o:	$(FILE_EMBEDDED_IN_CODE_DIR)/%.c
	$(VB)if echo "$(CC)" | grep -q "wasi"; then \
		echo [Compiling $<]; \
		$(CC) $(CFLAGS) -c $< -o $@; \
	else \
		echo [Delegating to WASI SDK Docker image]; \
		${RUN_IN_WASI_SDK_DOCKER_IMAGE} make $(MAKEFLAGS) $@; \
	fi

# Provide the Doom shareware WAD by retrieving it via the URL advertised here: https://doomwiki.org/wiki/DOOM1.WAD
$(OUTPUT_DIR)/DOOM1.WAD: | $(OUTPUT_DIR)
	@echo [Retreiving Shareware Doom WAD from the internet]
	$(VB)curl --output $(OUTPUT_DIR)/DOOM1.WAD https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad

# Support generating .c and .h files that contain and reference, respectively, an embedded copy of another file
$(FILE_EMBEDDED_IN_CODE_DIR)/%.c $(FILE_EMBEDDED_IN_CODE_DIR)/%.h: $(OUTPUT_DIR)/% utils/generate_code_for_embedded_file.py | $(FILE_EMBEDDED_IN_CODE_DIR) ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Here's what we're depending on: ${DEV_PYTHON_VIRTUAL_ENV}]
	@echo [Generating $(<F).c and $(<F).h, the source and header file to embed '$<']
	${VB}( \
		${ACTIVATE_DEV_PYTHON_VIRTUAL_ENV}; \
		python utils/generate_code_for_embedded_file.py --input $< --destination-folder $(FILE_EMBEDDED_IN_CODE_DIR) ; \
	)

# Compile .wat files in src/wat in order to produce an associated .wasm module in $(OUTPUT_DIR)/util-wasm-modules
$(OUTPUT_UTIL_WASM_MODULES)/%.wasm: src/wat/%.wat $(WASM_AS) | $(OUTPUT_UTIL_WASM_MODULES)
	@echo [Compiling WAT file \'$<\']
	$(VB)$(WASM_AS) $< -o $@

BINARYEN_FLAGS = --enable-bulk-memory

# After compilation and linking has finished the Doom WebAssembly module passes through a few custom
# transformations before it's considered complete:
#
#
#   1. All needed `wasi_snapshot_preview1` imports are filled by calling back into similarly named exports
OUTPUT_INTERMEDIATE_WITH_WASI_HOLES_FILLED = $(OUTPUT_DIR)/doom-with-wasi-holes-filled.wasm
$(OUTPUT_INTERMEDIATE_WITH_WASI_HOLES_FILLED): $(OUTPUT_RAW_FROM_LINKING) $(OUTPUT_UTIL_WASM_MODULES)/wasi_snapshot_preview1-trampolines.wasm $(WASM_MERGE)
	@echo [Merging the Doom WebAssembly module with wasi-snapshot-preview1 trampolines]
	$(VB)$(WASM_MERGE) $< wasi-implementation $(word 2,$^) wasi_snapshot_preview1 -o $@ $(BINARYEN_FLAGS)
#
#
#   2. Multiple `initialization` functions are merged into a single one
OUTPUT_INTERMEDIATE_WITH_INIT_FUNCTIONS_MERGED = $(OUTPUT_DIR)/doom-with-init-functions-merged.wasm
$(OUTPUT_INTERMEDIATE_WITH_INIT_FUNCTIONS_MERGED): $(OUTPUT_INTERMEDIATE_WITH_WASI_HOLES_FILLED) $(OUTPUT_UTIL_WASM_MODULES)/merge-two-initialization-functions-into-one.wasm $(WASM_MERGE)
	@echo [Merging the Doom WebAssembly module with module that combines both init functions]
	$(VB)$(WASM_MERGE) $< has-two-init-functions $(word 2,$^) merges-init-functions -o $@ $(BINARYEN_FLAGS)
#
#
#   3. All exports that are not allow-listed are removed
OUTPUT_INTERMEDIATE_WITH_TRIMMED_EXPORTS = $(OUTPUT_DIR)/doom-with-trimmed-exports.wasm
$(OUTPUT_INTERMEDIATE_WITH_TRIMMED_EXPORTS): $(OUTPUT_INTERMEDIATE_WITH_INIT_FUNCTIONS_MERGED) src/reachability_graph_for_wasm-metadce.json $(WASM_METADCE)
	@echo [Removing from Doom WebAssembly module all exports not listed as reachable in $(word 2,$^)]
#     Note: the wasm-metadce tool is very chatty, unconditionally (as far as I can tell) outputing details
#     about the unused exports. To prevent this mostly useless output from being seen we redirect stdout to
#     /dev/null unless VERBOSE is set to something other than 0.
	$(VB)$(WASM_METADCE) $< --graph-file $(word 2,$^) -o $@ $(BINARYEN_FLAGS) $(if $(VERBOSE:0=),,> /dev/null)
#
#
#   4. Many global constants are added
#				- Why? Clang/llvm toolchain does not currently support exporting global constants from C source code.
#					So we provide useful global constants to our users by merging them into the module this way instead.
OUTPUT_INTERMEDIATE_WITH_GLOBAL_CONSTANTS_ADDED = $(OUTPUT_DIR)/doom-with-global-constants-added.wasm
$(OUTPUT_INTERMEDIATE_WITH_GLOBAL_CONSTANTS_ADDED): $(OUTPUT_INTERMEDIATE_WITH_TRIMMED_EXPORTS) $(OUTPUT_UTIL_WASM_MODULES)/global-constants.wasm $(WASM_MERGE)
	@echo [Augmenting the Doom WebAssembly module with some global constants]
	$(VB)$(WASM_MERGE) $< doom $(word 2,$^) global-constants -o $@ $(BINARYEN_FLAGS)

$(OUTPUT): $(OUTPUT_INTERMEDIATE_WITH_GLOBAL_CONSTANTS_ADDED)
	@echo [Producing final Doom WebAssembly]
	$(VB)cp $< $@


# Produce a text file that describes the imports and exports of the Doom WebAssembly module.
$(OUTPUT_NAME).interface.txt: $(OUTPUT) utils/print-interface-of-wasm-module/
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
	${VB}$(MAKE) --directory=utils/print-interface-of-wasm-module/ run PATH_TO_WASM_MODULE=$(abspath $(OUTPUT)) >> $@


####################################################################
# Targets for building and running examples
####################################################################

# Each directory in "examples/" is expected to represent an example of
# how to make use of the Doom WebAssemly modulue.
#
# Such an 'example' should contain a Makefile in its directory that provides
# the two targets `build` and `run`. Either of these targets can depend upon the
# environment varaible PATH_TO_DOOM_WASM being set to point to the Doom
# WebAssembly module (and an example will leverage PATH_TO_DOOM_WASM in order
# to read the Doom WebAssembly module).

# Here we provide two targets per example that make it easy to either `build` or
# `run` that example via this Makefile:
#
#    run-example_$(example_name)
#    build-example_$(example_name)

EXAMPLES_DIR = examples
EXAMPLES = $(subst /,,$(subst $(EXAMPLES_DIR)/,,$(wildcard $(EXAMPLES_DIR)/*/)) )
EXAMPLES_BUILD = $(addprefix build-example_, $(EXAMPLES))
EXAMPLES_RUN = $(addprefix run-example_, $(EXAMPLES))

$(EXAMPLES_BUILD): build-example_%: $(OUTPUT)
	@echo [Building the example \'$*\']
	${VB}$(MAKE) --directory=examples/$* build PATH_TO_DOOM_WASM=$(abspath $(OUTPUT))

$(EXAMPLES_RUN): run-example_%: $(OUTPUT)
	@echo [Running the example \'$*\']
	${VB}$(MAKE) --directory=examples/$* run PATH_TO_DOOM_WASM=$(abspath $(OUTPUT))


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
