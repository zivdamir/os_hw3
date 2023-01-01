cmake_minimum_required(VERSION 3.16)
project(webserver_files C)

set(CMAKE_C_STANDARD 99)

add_executable(server server.c segel.c segel.h request.c request.h queue.c queue.h)
add_executable(client client.c segel.c segel.h queue.c queue.h)
target_link_libraries(server pthread)
target_link_libraries(client pthread)