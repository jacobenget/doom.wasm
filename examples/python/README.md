# Python Example

Here lives an example of leveraging the _Doom_ WebAssembly module (`doom.wasm`) produced by this repo in order to run _Doom_ via [Python](https://www.python.org/), making use of the [Wasmtime](https://wasmtime.dev/) WebAssembly runtime and [PyGame](https://www.pygame.org/wiki/about) in the process.

This example leverages all the features supported by `doom.wasm`. Particularly, this example supports the loading of custom WADs into _Doom_, and supports the user saving their game progress.

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
