cmake_minimum_required (VERSION 3.16)

project("RakNet")

add_library("RakNet" STATIC "src/AsynchronousFileIO.cpp"
                            "src/BitStream.cpp"
                            "src/CheckSum.cpp"
                            "src/DataBlockEncryptor.cpp"
                            "src/DataCompressor.cpp"
                            "src/DS_ByteQueue.cpp"
                            "src/DS_HuffmanEncodingTree.cpp"
                            "src/DS_Table.cpp"
                            "src/EncodeClassName.cpp"
                            "src/GetTime.cpp"
                            "src/InternalPacketPool.cpp"
                            "src/LinuxStrings.cpp"
                            "src/NetworkIDGenerator.cpp"
                            "src/NetworkTypes.cpp"
                            "src/PluginInterface.cpp"
                            "src/RakClient.cpp"
                            "src/RakNetStatistics.cpp"
                            "src/RakNetworkFactory.cpp"
                            "src/RakPeer.cpp"
                            "src/rakserver.cpp"
                            "src/RakSleep.cpp"
                            "src/Rand.cpp"
                            "src/ReliabilityLayer.cpp"
                            "src/rijndael.cpp"
                            "src/RPCMap.cpp"
                            "src/SHA1.cpp"
                            "src/SimpleMutex.cpp"
                            "src/SocketLayer.cpp"
                            "src/StringCompressor.cpp"
                            "src/StringTable.cpp"
                            "src/SystemAddressList.cpp"
                            "src/TableSerializer.cpp"
                            "src/_findfirst.cpp"
                            "src/sampencr.cpp")


set_target_properties("RakNet"
    PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

target_compile_definitions("RakNet" PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE)

target_include_directories("RakNet" PRIVATE "include/RakNet")
target_include_directories("RakNet" PUBLIC "include")

target_link_libraries("RakNet" PUBLIC ws2_32 comctl32)