cmake_minimum_required(VERSION 3.16)

project(Lab3a)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server src/Server.cpp)
add_executable(client src/Client.cpp)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
endif()
