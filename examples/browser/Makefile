# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif

# As part of an 'example' in the doom.wasm project, this Makefile is allowed to
# depend upon the environment variable ABSOLUTE_PATH_TO_DOOM_WASM being set and
# in return is expected to provide these two make targets:
#
# 	build
#			- performs any upfront work and validation to make sure this example can 'run'
#		run
#			- executes the example in some way

ASSETS = assets
DOOM_WASM = $(ASSETS)/doom.wasm

all: build

$(ASSETS):
	@echo [Creating output folder \'$@\']
	$(VB)mkdir -p $@

$(DOOM_WASM): $(ABSOLUTE_PATH_TO_DOOM_WASM) | ensure-path-to-doom_wasm-is-set $(ASSETS)
	@echo [Copying artifact locally to \'$@\']
	$(VB)cp $< $@

build: $(DOOM_WASM)

PORT = 8000
run: $(DOOM_WASM)
	@echo [Serving up Doom in HTML, visit 127.0.0.1:$(PORT)/doom.html to see it]
	$(VB)python3 -m http.server $(PORT)

ensure-path-to-doom_wasm-is-set:
ifndef ABSOLUTE_PATH_TO_DOOM_WASM
	$(error ABSOLUTE_PATH_TO_DOOM_WASM is undefined)
endif

.PHONY: ensure-path-to-doom_wasm-is-set run build all
