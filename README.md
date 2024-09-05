# doom.wasm

This project's aim is to compile the Doom source to WebAssembly, producing a module with a small and easy-to-understand interface.

This project takes advantage of much work done previously by others to make the Doom source code easy to build and easy to interface with. Particularly, the work done in these projects built the ground upon which this project stands:=
[doomgeneric](https://github.com/ozkl/doomgeneric), [fbDoom](https://github.com/maximevince/fbDOOM), [Frosted Doom](https://github.com/insane-adding-machines/DOOM), and [Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom).

## Development

This project leverages [`pre-commit`](https://pre-commit.com/) to enforce certain standards in this codebase.

In order to take advantage of this pre-commit check locally you'll have to register the appropriate hooks with git.

This is accomplished by initializing the dev setup for this repo via this command:

```
make dev-init
```
