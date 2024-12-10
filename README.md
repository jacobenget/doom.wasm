# `doom.wasm`

Providing a [WebAssembly](https://webassembly.org/) module that can fully run [_Doom_](https://en.wikipedia.org/wiki/Doom_(1993_video_game)) while having a small and easy-to-understand interface.

[Here](https://jacobenget.github.io/doom.wasm/examples/browser/doom.html) is a bare-bones (i.e. the source code is very simple, take a look!) example of using this WebAssembly module to run _Doom_ in the browser - game controls are the same keyboard controls present in vanilla _Doom_, detailed [here](https://doom.fandom.com/wiki/Controls#Default_controls).

## Why?

Wait, hasn't _Doom_ already been ported to WebAssembly?

Yes. Definitely. So many times.

This issue is that past attempts to compile _Doom_ to WebAssembly have only had the "Human who wants to play _Doom_" user in mind. These attempts demonstrate running _Doom_ somewhere interesting (e.g. in a browser) but fail to be more than a tech or research demo.

In particular, none of these past attempts produced a _Doom_ WebAssembly module that can be easily leveraged to, using any WebAssembly runtime, run _Doom_ in a new way.

Look at a measure of interface "surface area" of the WebAssembly modules produced by a few "_Doom_ running on WebAssembly" projects in the wild:

| Project | Imported Functions | Exported Functions | Surface Area (Imported and Exported Functions) | Notes |
| ---- | ---- | ---- | ---- | ---- |
| [cloudflare/doom-wasm](https://github.com/cloudflare/doom-wasm) | 259 | 62 | 321 | Powers the app hosted [here](https://silentspacemarine.com/). |
| [lazarv/wasm-doom](https://github.com/lazarv/wasm-doom) | 248 | 32 | 280 | Powers the [WAD Commander](https://wadcmd.com/) tool. |
| [diekmann/wasm-fizzbuzz/doom](https://github.com/diekmann/wasm-fizzbuzz/tree/main/doom#but-can-it-run-doom) | 5 | 48 | 53 | Powers the app hosted [here](https://diekmann.github.io/wasm-fizzbuzz/doom/). Doesn't support the core _Doom_ feature of loading custom WAD data. |
| [UstymUkhman/webDOOM](https://github.com/UstymUkhman/webDOOM) | 365 | 39 | 404 | Powers the app hosted [here](http://35.158.218.205/experiments/webDOOM/). |
| [raz0red/webprboom](https://github.com/raz0red/webprboom) | 372 | 43 | 415 | Powers the app hosted [here](https://raz0red.github.io/webprboom/). |
| [VanIseghemThomas/wasmDOOM](https://github.com/VanIseghemThomas/wasmDOOM) | 247 | 15 | 262 | Must be built from source |

Understanding anywhere from 50 to 400+ different functions defined by a WebAssembly module, either to implement or use, is not an easy task!

We can do better, and hope to keep the spirit of _Doom_'s impressive portability alive by compiling _Doom_ to a WebAssembly module that has a minimal and curated interface.

We've succeeded in producing a _Doom_ WebAssembly module that only imports 10 functions and exports 4 functions, resulting in an interface "surface area" of 14!

| Project | Imported Functions | Exported Functions | Surface Area (Imported and Exported Functions) | Notes |
| ---- | ---- | ---- | ---- | ---- |
| `doom.wasm` | 10 | 4 | 14 | Powers the app hosted [here](https://jacobenget.github.io/doom.wasm/examples/browser/doom.html). Supports loading of custom WAD data. |

Further information on this simple interface is located in [Details](#details).

Also, the maximum size of the WebAssembly `memory` needed by the _Doom_ WebAssembly module when it loads the [Doom Shareware WAD](https://doomwiki.org/wiki/DOOM1.WAD) is 248 WebAssembly memory `pages`, which are 2^16 bytes each, so this comes to about 16MB.

This means we've reduced the question

> [Can it run _Doom_?](https://canitrundoom.org/)

to

> Can it run a WebAssembly interpreter while having enough RAM for a 16MB WebAssembly memory?

## Building

Producing the WebAssembly module for _Doom_ (referred to as `doom.wasm` from now on), can be done via

```bash
make all
```

You will then find the module at `build/doom.wasm`.

### Requirements

Building this project requires that your system has these resources available:
- [Python](https://www.python.org/downloads/), version 3+
- `docker` (version `24.0.6` tested)
- `make` (GNU Make version `3.81` tested)
- `curl` (version `8.1.2` tested)
- `git` (version `2.43.0` tested)

## Running

The [`examples`](examples/) directory contains a few examples of using `doom.wasm` to run _Doom_.

Currently, there are three such examples:
1. [`browser`](examples/browser/): Runs _Doom_ in a webpage, using the browser's support for WebAssembly and drawing frames of _Doom_ to an HTML Canvas
   - This example is hosted live [here](https://jacobenget.github.io/doom.wasm/examples/browser/doom.html)
1. [`native`](examples/native/): Runs _Doom_ natively, leveraging the [Wasmtime](https://wasmtime.dev/) WebAssembly runtime and [SDL](https://www.libsdl.org/)
1. [`python`](examples/python/): Runs _Doom_ via [Python](https://www.python.org/), leveraging the [`wasmtime`](https://pypi.org/project/wasmtime/) Python bindings to the [Wasmtime](https://wasmtime.dev/) WebAssembly runtime, and [PyGame](https://www.pygame.org/wiki/about)

Each of these examples can be run from the top-level directory of this repo via a `make` target named `run-example_<example-name>`, e.g.:

```bash
make run-example_browser # start a local web server that hosts Doom in a webpage
```

```bash
make run-example_native # start Doom natively
```

```bash
make run-example_python # start Doom via Python
```

## Details

The interface of `doom.wasm` is comprised of:
- 10 imported functions
- 4 exported functions
- an exported `memory`
- 14 exported global constants (which exist purely to improve usability)

[`doom.wasm.interface.txt`](doom.wasm.interface.txt) contains a quick glance at the interface of `doom.wasm`. More details are provided below.

### Imports

A user of `doom.wasm` is expected to provide implementations of all these imported functions.

Note that almost all of these imports can have trivial or empty implementations if you have no use for the specific utility they provide!

| Function Name (including module prefix) | Expected Behavior | Notes |
| ---- | ---- | ---- |
| `loading.onGameInit` | Respond to _Doom_ first starting up | Perform any one-time initialization here |
| `loading.wadSizes` | Report size information about the WAD data that _Doom_ should load | Can do nothing, which communicates to _Doom_ "Load the [Doom Shareware WAD](https://doomwiki.org/wiki/DOOM1.WAD)" |
| `loading.readWads` | Copy to memory the data for all WAD files that _Doom_ should load, and the byte length of each WAD file | Only called if `loading.wadSizes` reported a greater-than-zero number of WADs to load |
| `runtimeControl.timeInMilliseconds` | Provide a representation of the current 'time', in milliseconds | |
| `ui.drawFrame` | Respond to a new frame of the _Doom_ game being available | |
| `gameSaving.sizeOfSaveGame` | Report the size, in bytes, of a specific save game | |
| `gameSaving.readSaveGame` | Copy data for a specific save game to memory | Only called if `gameSaving.sizeOfSaveGame` reported a save game with a non-zero size |
| `gameSaving.writeSaveGame` | Respond to the user attempting to save their game | Can just return `0` in the case that game saving isn't supported |
| `console.onInfoMessage` | Respond to _Doom_ reporting an info message | |
| `console.onErrorMessage` | Respond to _Doom_ reporting an error message | |

### Exports

#### Memory

The linear memory of _Doom_ is exported by `doom.wasm`.

This gives _Doom_ and the functions it has imported a shared space to read and write arbitrarily-sized data, e.g. when `ui.drawFrame` needs access to _Doom_'s frame buffer.

#### Functions

Four functions are exported by `doom.wasm`. The user should call these to run _Doom_.

| Function Name  | Behavior |
| ---- | ---- |
| `initGame()` | Initialize _Doom_; must be called before any other exported function is called |
| `tickGame()` | Advance _Doom_ by one 'tick' (i.e. one frame) |
| `reportKeyDown(doomKey: i32)` | Report to _Doom_ that a key is now pressed down |
| `reportKeyUp(doomKey: i32)` | Report to _Doom_ that a key is no longer pressed down |

#### Constants

Two functions exported by `doom.wasm`, `reportKeyDown` and `reportKeyUp`, accept an integer `doomKey` argument. This integer `doomKey` argument fills to role of representing all the "keys" that _Doom_ responds to.

But what values of `doomKey` should be associated with what keyboard keys?

Many "keys" that _Doom_ responds to (e.g. the keys `1` through `9`) are naturally associated with keyboard keys that produce a single printable character when pressed. In that case the `doomKey` value for that key is exactly the Unicode value representing the unmodified character generated by pressing that keyboard key. E.g. pass `49` to `reportKeyDown` when the player presses `1` on their keyboard.

Other "keys" that _Doom_ responds to either have Unicode values that aren't printable (e.g. `8`, for Unicode `backspace`) or have no Unicode equivalent (e.g. the "USE" key, which is used to open doors and flip switches in _Doom_).

To help in those cases, global constants with descriptive names have been exported by `doom.wasm`. Each global constant holds the `doomKey` value for one of these special "keys", and its this integer value that should be passed to `reportKeyDown` or `reportKeyUp` whenever the player has pressed or unpressed, respectively, the associated keyboard key.

Here's a table of all such global constants, along with a hint at what keyboard key might trigger such a "key" (based on [controls in vanilla _Doom_](https://doom.fandom.com/wiki/Controls)).

| Global Key Constant Name | Keyboard key vanilla _Doom_ (where not obvious) |
| ---- | ---- |
| `KEY_LEFTARROW` | |
| `KEY_RIGHTARROW` | |
| `KEY_UPARROW` | |
| `KEY_DOWNARROW` | |
| `KEY_STRAFE_L` | comma key: `,` |
| `KEY_STRAFE_R` | period key: `.` |
| `KEY_FIRE` | control key |
| `KEY_USE` | space bar |
| `KEY_SHIFT` | |
| `KEY_TAB` | |
| `KEY_ESCAPE` | |
| `KEY_ENTER` | |
| `KEY_BACKSPACE` | |
| `KEY_ALT` | |

#### Main Loop

After providing appropriate implementations for all functions imported by `doom.wasm`, and then instantiating the WebAssembly module, the common way you'd run _Doom_ is by calling `initGame()` once (required) followed by an unbounded number of calls to `tickGame()`, `reportKeyDown(doomKey)`, and `reportKeyUp(doomKey)`, in an infinite loop.

Here's pseudocode illustrating this:

```
doomExports.initGame()

while (True):
   doomExports.tickGame()

   for (keyStateChange in userInput):

      if keyStateChange.key is associated with a specialKey:
         doomKey = value stored in the associated global KEY_* constant
      else:
         doomKey = keyStateChange.key.unicodeValue

      if (keyStateChange.keyIsPressed):
         doomExports.reportKeyDown(doomKey)
      else:
         doomExports.reportKeyUp(doomKey)
```

See a concrete example of such a main loop in the implementation of `run_game` [here (C)](examples/native/src/imports_via_SDL/doom_imports.c) and in the implementation of `main` [here (Python)](examples/python/main.py).

Note that you could call `tickGame()` less aggressively than in this pseudocode.

The rate at which `tickGame()` is called does _not_ affect the rate at which time passes in the game. The passing of time in-game is instead controlled by the implementation of the imported function `runtimeControl.timeInMilliseconds`. The rate at which `tickGame()` is called just determines how often user input is processed and a frame of _Doom_ is rendered.

_Doom_ naturally wants to be rendered at 35 frames per second (this is the framerate of the game when it was first released in 1993), so calling `tickGame()` much less than 35 times per second will result in the game feeling sluggish. But feel free to call `tickGame()` _only_ 35 times a second, the game will still feel responsive.

### Further Details

The exact shape of all elements imported and exported by `doom.wasm` can be found in [`doom.wasm.interface.txt`](doom.wasm.interface.txt). This file is auto-generated on each commit, so it immediately surfaces any changes to the interface of `doom.wasm` caused by changes elsewhere. This should be considered an authority on the shape of the interface to `doom.wasm`.

The expected behavior and signature of all imports, and further details about how to use any exported function, lives in [src/doom_wasm.h](src/doom_wasm.h). This is the C header file declaring all functions exported and imported by _Doom_.

Also, both the [`native`](examples/native/) and [`python`](examples/python/) examples present in this repo contain full implementations of all imports needed by `doom.wasm`, and fully leverage all exports provided by `doom.wasm`. Studying these examples will likely fill in any gaps any related questions you might have.

## Development

This project leverages [`pre-commit`](https://pre-commit.com/) to enforce certain standards in this codebase.

In order to take advantage of this pre-commit check locally you'll have to register the appropriate hooks with git.

This is accomplished by initializing the dev setup for this repo via this command:

```
make dev-init
```

## TODO

There are many wishlist items worth accomplishing next with `doom.wasm`, but these missing pieces stand out enough to be worth mentioning (because you may be scratching your head about them):
1. Add support for music and sound effects
    - `doom.wasm` produces no music or sound effects when played!
    - The music of _Doom_ is iconic, and _Doom_ isn't _Doom_ without its aural components paired with its visual components
    - Adding music and sound effect support won't be trivial, likely complicated by the non-standard way that music and sound effects are stored in memory by _Doom_
2. Support "screen melt" effect in single-threaded environments
   - The ["screen melt" effect](https://doom.fandom.com/wiki/Screen_melt) is completely performed by _Doom_, from beginning to end, in a single call to `tickGame()`
   - This means that if `doom.wasm` is running on a platform where rendering of a frame of _Doom_ only happens when the main thread of execution is released (e.g. a browser) the user will not see any intermediate frames of this animation
   - Oh no!
   - Instead, a user on such a platform will experience a frozen game as they wait for the "screen melt" to complete behind the scenes, and for this very long call to `tickGame()` to return
   - This would be addressed by performing the animation of the "screen melt" asynchronously
   - A call to `tickGame()` where this animation starts or is in progress would have to return after rendering just _one_ frame of the animation, and then be capable of resuming the animation on the next call to `tickGame()`
3. Build from scratch in less time
   - Constructing `doom.wasm` requires a few [Binaryen](https://github.com/WebAssembly/binaryen) tools
   - The tools are built locally, and when built from scratch can take over an hour to build
   - This means that a build of `doom.wasm` from a fresh clone of this repo likely takes over an hour, ugh
   - Leverage a `docker` image containing pre-built Binaryen tools to seriously cut down this 'time to build from scratch'

## Credit Where Credit is Due

This project takes advantage of much work done previously by others to make the _Doom_ source code easy to build and easy to interface with.

Particularly, these other projects built the ground upon which this project stands:
[doomgeneric](https://github.com/ozkl/doomgeneric), [fbDoom](https://github.com/maximevince/fbDOOM), [Frosted Doom](https://github.com/insane-adding-machines/DOOM), and [Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom). Many thanks go to them!
