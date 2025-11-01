set(BOMB2_SRC statetbl.c sync_queue.c)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(bomb2 bomb2.c ${BOMB2_SRC})
target_compile_options(bomb2 PRIVATE -Wall -Wextra -pthread)

set(BOMB3_SRC sync_queue.c)
add_executable(bomb3 bomb3.cpp ${BOMB3_SRC})
target_compile_options(bomb3 PRIVATE -Wall -Wextra -pthread)

set(BOMB4_SRC sync_queue.c qfsm.c)
add_executable(bomb4 bomb4.c ${BOMB4_SRC})
target_compile_options(bomb4 PRIVATE -Wall -Wextra -pthread)