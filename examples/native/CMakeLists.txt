cmake_minimum_required(VERSION 3.23)
project(doom C)

find_package(wasmtime REQUIRED)
find_package(SDL2 REQUIRED)

# A library that uses SDL to implement all the imports needed by Doom.
set(ImportsViaSdlSrc
  src/imports_via_SDL/doom_imports.c
)

add_library(ImportsViaSdlLib STATIC ${ImportsViaSdlSrc})
target_include_directories(ImportsViaSdlLib PRIVATE src)
target_link_libraries(ImportsViaSdlLib SDL2::SDL2)

# A library that uses the Wasmtime WebAssembly interpreter to `implement` all the exports provided by Doom.
set(ExportsViaWasmtimeSrc
  src/exports_via_wasmtime/wrapped_func.h
  src/exports_via_wasmtime/wrapped_func.c
  src/exports_via_wasmtime/doom_exports.c
)

add_library(ExportsViaWasmtimeLib STATIC ${ExportsViaWasmtimeSrc})
target_include_directories(ExportsViaWasmtimeLib PRIVATE src)
target_link_libraries(ExportsViaWasmtimeLib wasmtime::wasmtime)

# The application ties together the imports and exports to make Doom playable
set(MainSrc
  src/doom_utils.h
  src/doom_imports.h
  src/doom_exports.h
  src/main.c
)

add_executable(${PROJECT_NAME} ${MainSrc})
target_link_libraries(${PROJECT_NAME} ImportsViaSdlLib ExportsViaWasmtimeLib)
