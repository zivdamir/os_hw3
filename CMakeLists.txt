cmake_minimum_required(VERSION 3.16)
project(webserver_files C)

set(CMAKE_C_STANDARD 99)

add_executable(server server.c segel.c segel.h request.c request.h)
add_executable(client client.c segel.c segel.h )
