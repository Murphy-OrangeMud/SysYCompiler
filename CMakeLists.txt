cmake_minimum_required(VERSION 3.5)
project("SysYCompiler" VERSION "0.0.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # ?

#add_compile_options(-Wall -Wno-error -O2)
add_compile_options(-O2 -lm -std=c++17 -Wno-register) # the same as oj

include_directories(src)

add_compile_definitions(APP_NAME="SysY Compiler")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(APP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(APP_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(APP_VERSION_PATCH=${PROJECT_VERSION_PATCH})

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(sysyc ${SOURCES})
