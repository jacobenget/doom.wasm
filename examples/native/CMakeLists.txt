cmake_minimum_required(VERSION 3.23)
project(doom C)

find_package(wasmtime REQUIRED)
find_package(SDL2 REQUIRED)

set(SourceList
  src/main.c
  src/wrapped_func.h
  src/wrapped_func.c
  src/doom_imports.h
  src/doom_imports.c
  src/user_input.h
  src/user_input.c
)

add_executable(${PROJECT_NAME} ${SourceList})
target_link_libraries(${PROJECT_NAME} wasmtime::wasmtime SDL2::SDL2)
