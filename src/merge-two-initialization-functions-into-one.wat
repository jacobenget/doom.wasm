;; This simple WebAssembly module provides and exports an 'initGame' function that is exactly
;; the result of calling two function, in order:
;;   1) an imported `_initialize()` function
;;   2) an imported `_initializeDoom()` function
;;
;; WHy does this module exist?
;;
;; This module, along with the use of the `wasm-merge` tool (https://github.com/WebAssembly/binaryen?tab=readme-ov-file#wasm-merge)
;; gives the ability to take a module that has this requirement:
;;
;;     - "To use this module first call `_initialize()` and then call `_initializeDoom()`"
;;
;; and turn it into a module that has this simpler, and harder to mess up, requirement:
;;
;;     - "To use this module first call `initGame()`"
;;
;; Note: An `_initialize` function is auto-generated and then exported when a module is built
;; as a WASI 'reactor', see here: https://github.com/WebAssembly/WASI/blob/main/legacy/application-abi.md#current-unstable-abi
;; The fact that the `_initialize` function doesn't exist until compiling and possiblying linking
;; has completed is why the aim to "provide an `initGame()` that does all initialization at once"
;; can't be done via source code that's part of the initial compilation/linking, and is instead
;; accomplished externally using the simple module you see here.
(module
 (type $0 (func))
 (import "has-two-init-functions" "_initialize" (func $fimport$0))
 (import "has-two-init-functions" "_initializeDoom" (func $fimport$1))
 (export "initGame" (func $0))
 (func $0
  (call $fimport$0)
  (call $fimport$1)
 )
)
