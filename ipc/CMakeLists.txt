cmake_minimum_required (VERSION 3.16)

project("connd-ipclib")

add_library("connd-ipclib" STATIC "src/ipc_server.cpp" "src/ipc_client.cpp")

target_include_directories("connd-ipclib" PUBLIC "include")

target_link_libraries("connd-ipclib" PUBLIC ws2_32)

set_target_properties("connd-ipclib"
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)