cmake_minimum_required (VERSION 3.16)

project("connd-sa")

find_package(magic_enum REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(plugin-sdk REQUIRED)
find_package(spdlog REQUIRED)

add_library("connd-sa" SHARED   "dllmain.cpp"
                                "BitStream.cpp"
                                "Utils.cpp")

set_target_properties("connd-sa"
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries("connd-sa" PRIVATE    kthook
                                            sampapi
                                            magic_enum::magic_enum
                                            nlohmann_json::nlohmann_json
                                            pluginsdk::gtasa 
                                            connd-ipclib
                                            RakNet
                                            spdlog::spdlog)

set_target_properties("connd-sa" PROPERTIES
  SUFFIX "-by-kin4stat.asi"
  #INTERPROCEDURAL_OPTIMIZATION TRUE
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>"
)