cmake_minimum_required(VERSION 3.15)

project(bomb)

set(BOMB2_SRC statetbl.c sync_queue.c)
set(CMAKE_BUILD_TYPE Debug)

add_executable(bomb2 bomb2.c ${BOMB2_SRC})
target_compile_options(bomb2 PRIVATE -Wall -Wextra -pthread)