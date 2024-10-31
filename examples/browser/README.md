# Browser Example

Here lives an example of leveraging the _Doom_ WebAssembly module (`doom.wasm`) produced by this repo in order to run _Doom_ in a browser.

This example leverages the minimal number of features in `doom.wasm` in order to run _Doom_.

Particularly, this example only manages these details in order to run _Doom_:
1. Draw the frames of _Doom_ to an HTML canvas
1. Report the current time to _Doom_ by using [`performance.now()`](https://developer.mozilla.org/en-US/docs/Web/API/Performance/now)
1. Listen to all [KeyboardEvent](https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent) activity on the HTML canvas and forward all relevant ones to _Doom_
1. Forward _Doom_ `info` and `error` messages to the JavaScript console (not necessary, but helpful!)

What's missing? This example doesn't support loading custom WADs into _Doom_ (instead _Doom_ always loads the [_Doom_ shareware WAD](https://doomwiki.org/wiki/DOOM1.WAD)), and doesn't support the user saving their game progress.

It wouldn't be difficult to add these missing features to this example, but they're being left out so this example is a demonstration of the minimal code needed to get _Doom_ running in this use case.

## Running

The `make` target `run` exists for "running" this example. Running this example means locally serving up a webpage that runs _Doom_.

To run this example one must provide a path to a copy of `doom.wasm`, as produced by this repo, via the `PATH_TO_DOOM_WASM` env variable.

If you've built `doom.wasm` locally then such a path, relative to here, would be `../../build/doom.wasm`. If that's the case then here's how you'd start up a web server that hosts _Doom_ in a webpage:

```bash
make run PATH_TO_DOOM_WASM=../../build/doom.wasm
```

After you've done that you can open [127.0.0.1:8000/doom.html](http://127.0.0.1:8000/doom.html) in a browser and play _Doom_!
