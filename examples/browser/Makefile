# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif

# As part of an 'example' in the doom.wasm project, this Makefile is expected to
# provide these two make targets:
#
#		build
#			- performs any upfront work and validation to make sure this example can 'run'
#			- can find the Doom WebAssembly module via the value of the PATH_TO_DOOM_WASM env var
#		run
#			- executes the example in some way
#			- can find the Doom WebAssembly module via the value of the PATH_TO_DOOM_WASM env var

ASSETS = assets
LOCAL_COPY_OF_DOOM_WASM = $(ASSETS)/doom.wasm

all: build

# Nothing to actual do on 'build'
build:

$(ASSETS):
	@echo [Creating output folder \'$@\']
	$(VB)mkdir -p $@

# Unconditionally (because this is a .PHONY target) save a local copy of the Doom WebAssembly
# artifact to the path where the HTTP server expects to serve up this artifact.
$(LOCAL_COPY_OF_DOOM_WASM): | ensure-path-to-doom_wasm-is-properly-set $(ASSETS)
	@echo [Copying artifact locally to \'$@\']
	$(VB)cp $(PATH_TO_DOOM_WASM) $@

PORT = 8000
run: $(LOCAL_COPY_OF_DOOM_WASM)
	@echo [Serving up Doom in HTML, visit 127.0.0.1:$(PORT)/doom.html to see it]
	$(VB)python3 -m http.server $(PORT)

ensure-path-to-doom_wasm-is-properly-set:
ifndef PATH_TO_DOOM_WASM
	$(error PATH_TO_DOOM_WASM is undefined, this should be set to point to the Doom Webassembly module)
else ifeq (,$(wildcard $(abspath $(PATH_TO_DOOM_WASM))))
	$(error PATH_TO_DOOM_WASM ('$(PATH_TO_DOOM_WASM)') does not point to a file that exists)
endif

.PHONY: run build all ensure-path-to-doom_wasm-is-properly-set $(LOCAL_COPY_OF_DOOM_WASM)
