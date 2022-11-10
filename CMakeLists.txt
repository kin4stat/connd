cmake_minimum_required (VERSION 3.16)

project("connd")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

add_compile_definitions(NOMINMAX)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

add_subdirectory("ipc")
add_subdirectory("RakNet")
add_subdirectory("third-party")
add_subdirectory("sa_module")
add_subdirectory("daemon")
