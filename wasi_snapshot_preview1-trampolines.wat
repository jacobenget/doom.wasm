;; This simple WebAssembly module implements and exports functions by directly calling a similarly
;; imported function, e.g. "X" is implemented entirely by calling a function named "internal.internal__X".
;;
;; It does this for the following WASI snapshot preview1 functions:
;;   fd_close
;;   fd_fdstat_get
;;   fd_seek
;;   fd_write
;;   proc_exit
;;
;;
;; WHy does this module exist?
;;
;; This module, along with the use of the binaryen tool `wasm-merge` (https://github.com/WebAssembly/binaryen?tab=readme-ov-file#wasm-merge),
;; gives another module the ability to provides its own implementation of these WASI snapshot preview1 functions.
;;
;; How? A module can implement the above mentioned WASI snapshot preview1 functions, prefixing all the function names with "internal__",
;; and then that module shall be merged with this module (via wasm-merge). The resultant merged module will now call the related "internal__"
;; function whenever a wasi function is called (e.g. a call to "fd_close" will delegate to "internal__fd_close").
(module
 (type $0 (func (param i32) (result i32)))
 (type $1 (func (param i32 i32) (result i32)))
 (type $2 (func (param i32 i64 i32 i32) (result i32)))
 (type $3 (func (param i32 i32 i32 i32) (result i32)))
 (type $4 (func (param i32)))
 (import "wasi-implementation" "internal__fd_close" (func $fimport$0 (param i32) (result i32)))
 (import "wasi-implementation" "internal__fd_fdstat_get" (func $fimport$1 (param i32 i32) (result i32)))
 (import "wasi-implementation" "internal__fd_seek" (func $fimport$2 (param i32 i64 i32 i32) (result i32)))
 (import "wasi-implementation" "internal__fd_write" (func $fimport$3 (param i32 i32 i32 i32) (result i32)))
 (import "wasi-implementation" "internal__proc_exit" (func $fimport$4 (param i32)))
 (export "fd_close" (func $0))
 (export "fd_fdstat_get" (func $1))
 (export "fd_seek" (func $2))
 (export "fd_write" (func $3))
 (export "proc_exit" (func $4))
 (func $0 (param $0 i32) (result i32)
  (call $fimport$0
   (local.get $0)
  )
 )
 (func $1 (param $0 i32) (param $1 i32) (result i32)
  (call $fimport$1
   (local.get $0)
   (local.get $1)
  )
 )
 (func $2 (param $0 i32) (param $1 i64) (param $2 i32) (param $3 i32) (result i32)
  (call $fimport$2
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
  )
 )
 (func $3 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (result i32)
  (call $fimport$3
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
  )
 )
 (func $4 (param $0 i32)
  (call $fimport$4
   (local.get $0)
  )
 )
)
