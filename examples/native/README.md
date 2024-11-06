# Native Example

Here lives an example of leveraging the _Doom_ WebAssembly module (`doom.wasm`) produced by this repo in order to run _Doom_ in a native environment.

This example leverages all the features supported by `doom.wasm`. Particularly, this example supports the loading of custom WADs into _Doom_, and supports the user saving their game progress.

## Code Organization

The code for this example is strongly separated into two halves. Each half is only exposed to the other half via implementation-agnostic C header files.

The two halves are:
1. **Exports** provided by _Doom_ WebAssembly module
   - Implements the interface described by [doom_exports.h](src/doom_exports.h)
   - Uses a WebAssembly runtime to load `doom.wasm` and provides access to the functions and global constants exported by `doom.wasm`
   - Currently implemented in C using [Wasmtime](https://wasmtime.dev/), in [src/exports_via_wasmtime/](src/exports_via_wasmtime/)
1. **Imports** needed by _Doom_ WebAssembly module
   - Implements the interface described by [doom_imports.h](src/doom_imports.h)
   - Provides implementations of all functions imported by `doom.wasm`, which includes all the ways that _Doom_ interacts with the outside world (e.g. drawing a frame to the screen), and also provides an implementation of a main event loop that calls the exported functions `tickDoom`, `reportKeyDown`, and `reportKeyUp`
   - Current implemented in C using [SDL](https://www.libsdl.org/), in [src/imports_via_SDL/](src/exports_via_wasmtime/)

These two halves, strongly separated, exist to make it easier to change the implementation of one of the halves while not touching the other.

Want to use a different WebAssembly runtime than Wasmtime, or use a different (e.g. C++ or Rust) interface to Wasmtime? You'll just have to touch the implementation of [doom_exports.h](src/doom_exports.h).

Want to use a different UI library than SDL, or instead draw the frames of _Doom_ to the console via colored ASCII, or simulate user input in some novel way? You'll just have to touch the implementation of [doom_imports.h](src/doom_imports.h).

## Building

Building this example is done via

```bash
make build
```

### Requirements

Building this project has these additional requirements on your system, along with the usual requirements required by this repo:
- C compiler, findable by `CMake`

## Running

The `make` target `run` exists for running this example.

To run this example one must provide a path to a copy of `doom.wasm`, as produced by this repo, via the `PATH_TO_DOOM_WASM` env variable.

If you've built `doom.wasm` locally then such a path, relative to here, would be `../../build/doom.wasm`. If that's the case then here's how you'd run this example:

```bash
make run PATH_TO_DOOM_WASM=../../build/doom.wasm
```

There is also a `make` target (`run-with-a-custom-pwad`) that serves as a demonstration of how to load a custom WAD into _Doom_ when running this example. This is done like this:

```bash
make run-with-a-custom-pwad PATH_TO_DOOM_WASM=../../build/doom.wasm
```
