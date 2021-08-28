cmake_minimum_required(VERSION 3.10)

project(input-socket VERSION 0.1 DESCRIPTION "Unix Socket listening for Events" LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(TARGET_GROUP production CACHE STRING "Build?")

add_subdirectory(src)

if(TARGET_GROUP STREQUAL release)
    # release things
    add_executable(input-socket src/unix_socket.c)
#add_executable(input-socket src/queue_test.c src/queue.h)
elseif(TARGET_GROUP STREQUAL test)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
else()
    message(FATAL_ERROR "can build either release or test")
endif()

