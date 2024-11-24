# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif

# As part of an 'example' in the doom.wasm project, this Makefile is expected to
# provide these two make targets:
#
#   build
#     - performs any upfront work and validation to make sure this example can 'run'
#     - can find the Doom WebAssembly module via the value of the PATH_TO_DOOM_WASM env var
#   run
#     - executes the example in some way
#     - can find the Doom WebAssembly module via the value of the PATH_TO_DOOM_WASM env var

PYTHON_DEV_VIRTUAL_ENV = .python_dev_virtualenv
PYTHON_DEV_REQUIREMENTS_TXT = python_dev_requirements.txt
ACTIVATE_PYTHON_DEV_VIRTUAL_ENV = source $(PYTHON_DEV_VIRTUAL_ENV)/bin/activate

OUTPUT_DIR = build
OUTPUT_EXECUTABLE = $(OUTPUT_DIR)/doom

all: build

dev-clean:
	$(VB) rm -fr $(PYTHON_DEV_VIRTUAL_ENV)
	$(VB) rm -fr $(OUTPUT_DIR)
	$(VB) rm -f CMakeUserPresets.json

$(PYTHON_DEV_VIRTUAL_ENV):
	@echo [Creating Python virtual environment at '$(PYTHON_DEV_VIRTUAL_ENV)/']
	$(VB)python3 -m venv $(PYTHON_DEV_VIRTUAL_ENV)
	$(VB)( \
		$(ACTIVATE_PYTHON_DEV_VIRTUAL_ENV); \
		echo [Initialize Python virtual environment with all needed dependencies]; \
		python -m pip install -r $(PYTHON_DEV_REQUIREMENTS_TXT); \
	)

# Update Python dev requirements file to match the package versions being used in the Python dev environment
# (this is done via a .PHONY target because it's not clear how we could direct 'make' to determine when this
# file is truly out of date, so instead we just always assume this file is out of date)
generate-python-dev-requirements: | $(PYTHON_DEV_VIRTUAL_ENV)
	$(VB)> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "##################################################################################################" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "#" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# This file is used to manage which Python modules are installed in the virtual environment that lives at $(DEV_PYTHON_DEV_VIRTUAL_ENV)/" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "#" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# WARNING: This file was auto-generated, likely by calling something similar to 'make $@'." >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# Any manual edits made to this file will probably be clobbered!" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "#" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# If you wish to change which Python modules are installed in the virtual environment at $(DEV_PYTHON_DEV_VIRTUAL_ENV)/" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# you should, instead, just manually install/uninstall modules in that virtual environment as you see fit" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# (see here if you're unsure of how to interact with Python virtual environment: https://docs.python.org/3/tutorial/venv.html)" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "# and then manually run 'make $@'." >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "#" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "##################################################################################################" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	@echo "" >> $(PYTHON_DEV_REQUIREMENTS_TXT)
	$(VB)( \
		$(ACTIVATE_PYTHON_DEV_VIRTUAL_ENV); \
		python -m pip freeze >> $(PYTHON_DEV_REQUIREMENTS_TXT); \
	)

CONAN_INSTALL = $(OUTPUT_DIR)/marker_file_from_when_conon_install_last_ran.empty

$(CONAN_INSTALL): conanfile.txt | $(PYTHON_DEV_VIRTUAL_ENV)
	$(VB)@echo [Removing any previous Conan installation]
	$(VB)rm -fr $(OUTPUT_DIR)
	$(VB)rm -f CMakeUserPresets.json
	$(VB)@echo [Creating output folder \'$(OUTPUT_DIR)\']
	$(VB)mkdir -p $(OUTPUT_DIR)
	$(VB)@echo [Creating marker file \'$@\']
	$(VB)touch $@
	$(VB)( \
		$(ACTIVATE_PYTHON_DEV_VIRTUAL_ENV); \
		echo [Ensure that a default Conan profile exists]; \
		conan profile detect --exist-ok; \
		echo [Install all resources necessary to build requirements specified in conanfile.txt]; \
		conan install . --output-folder=$(OUTPUT_DIR) --build=missing -s build_type=Debug; \
	)

CMakeUserPresets.json: $(CONAN_INSTALL)

CMAKE_BUILD_GENERATION = $(OUTPUT_DIR)/marker_file_from_when_cmake_build_generation_last_ran.empty

$(CMAKE_BUILD_GENERATION): CMakeLists.txt CMakeUserPresets.json $(CONAN_INSTALL) | $(PYTHON_DEV_VIRTUAL_ENV)
	$(VB)( \
		@echo [Creating marker file \'$@\']; \
		touch $@; \
		$(ACTIVATE_PYTHON_DEV_VIRTUAL_ENV); \
		echo [Generate a build system for this project]; \
		cmake --preset conan-debug; \
	)


build: | $(PYTHON_DEV_VIRTUAL_ENV) $(CMAKE_BUILD_GENERATION)
	$(VB)( \
		$(ACTIVATE_PYTHON_DEV_VIRTUAL_ENV); \
		cmake --build $(OUTPUT_DIR); \
	)

clean: | $(PYTHON_DEV_VIRTUAL_ENV) $(CMAKE_BUILD_GENERATION)
	$(VB)( \
		$(ACTIVATE_PYTHON_DEV_VIRTUAL_ENV); \
		cmake --build $(OUTPUT_DIR) --target clean; \
	)

$(OUTPUT_EXECUTABLE): build

run: $(OUTPUT_EXECUTABLE) | ensure-path-to-doom_wasm-is-properly-set
	$(VB)$< $(PATH_TO_DOOM_WASM)

# Provide a make target that runs Doom with a specific custom WAD
#
# And have that custom WAD (and an IWAD that supports the custom WAD) be downloaded on demand

WAD_FILE_DIR = wad_files

$(WAD_FILE_DIR):
	@echo [Creating folder for WAD files: \'$@\']
	$(VB)mkdir -p $@

$(WAD_FILE_DIR)/freedoom2.wad: | $(WAD_FILE_DIR)
	@echo [Downloading Freedoom2 IWAD]
	$(VB)curl --location --output $@ https://github.com/redox-os/freedoom/raw/refs/heads/master/freedoom2.wad

$(WAD_FILE_DIR)/GOTHIC2.WAD: | $(WAD_FILE_DIR)
	@echo [Downloading GOTHIC2 PWAD]
	$(VB)curl --location --output $(WAD_FILE_DIR)/gothic2.zip https://www.gamers.org/pub/idgames/levels/doom2/deathmatch/megawads/gothic2.zip
	$(VB)unzip -d $(WAD_FILE_DIR) $(WAD_FILE_DIR)/gothic2.zip GOTHIC2.WAD
	$(VB)rm $(WAD_FILE_DIR)/gothic2.zip

run-with-a-custom-pwad: $(OUTPUT_EXECUTABLE) $(WAD_FILE_DIR)/freedoom2.wad $(WAD_FILE_DIR)/GOTHIC2.WAD | ensure-path-to-doom_wasm-is-properly-set
	@echo [Running Doom with GOTHIC2 WAD on top of Freedoom2]
	$(VB)$< $(PATH_TO_DOOM_WASM) $(WAD_FILE_DIR)/freedoom2.wad $(WAD_FILE_DIR)/GOTHIC2.WAD

ensure-path-to-doom_wasm-is-properly-set:
ifndef PATH_TO_DOOM_WASM
	$(error PATH_TO_DOOM_WASM is undefined, this should be set to point to the Doom Webassembly module)
else ifeq (,$(wildcard $(abspath $(PATH_TO_DOOM_WASM))))
	$(error PATH_TO_DOOM_WASM ('$(PATH_TO_DOOM_WASM)') does not point to a file that exists)
endif

.PHONY: all dev-init dev-clean generate-python-dev-requirements build clean run ensure-path-to-doom_wasm-is-properly-set
