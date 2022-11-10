cmake_minimum_required (VERSION 3.16)

project("connd-daemon")

find_package(magic_enum REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable("connd-daemon" "main.cpp")

set_target_properties("connd-daemon"
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries("connd-daemon" PRIVATE    magic_enum::magic_enum
                                                nlohmann_json::nlohmann_json
                                                RakNet
                                                connd-ipclib)
set_target_properties("connd-daemon" PROPERTIES
  SUFFIX "-by-kin4stat.exe"
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>"
)