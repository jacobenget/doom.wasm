VIRTUAL_ENV_ACTIVATE_SCRIPT = .rust_virtual_env_activate

ACTIVATE_RUST_VIRTUAL_ENV = source $(VIRTUAL_ENV_ACTIVATE_SCRIPT)

all: build

build:
	@( \
		$(ACTIVATE_RUST_VIRTUAL_ENV); \
		cargo build; \
	)

run:
	@( \
		$(ACTIVATE_RUST_VIRTUAL_ENV); \
		cargo run $(PATH_TO_WASM_MODULE); \
	)
