cmake_minimum_required(VERSION 3.10)
include(CMakePrintHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

project(input-socket VERSION 0.1 DESCRIPTION "Unix Socket listening for Events" LANGUAGES C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)

set(TARGET_GROUP production CACHE STRING "Build?")

add_subdirectory(src)

if(TARGET_GROUP STREQUAL release)
    # release things
    # Ahem, I believe there is better way than just this ever expanding long list of extra c files
    add_executable(uv_socket src/uv_socket.c src/path/path.c src/path/path.h src/queue/queue.h src/queue/queue.c src/cramont/cramont.h src/cramont/cramont.c)

    pkg_check_modules(LIBX11 REQUIRED x11)
    set(RAMONT_LIBS "${LIBX11_LIBRARIES}")
    set(RAMONT_FLAGS "${LIBX11_FLAGS}")
    set(RAMONT_INCLUDE_DIRS "${LIBX11_INCLUDE_DIRS}")

    pkg_check_modules(LIBEVDEV REQUIRED libevdev)
    list(APPEND RAMONT_LIBS "${LIBEVDEV_LIBRARIES}")
    list(APPEND RAMONT_FLAGS "${LIBEVDEV_FLAGS}")
    list(APPEND RAMONT_INCLUDE_DIRS "${LIBEVDEV_INCLUDE_DIRS}")

    pkg_check_modules(CJSON REQUIRED libcjson)
    list(APPEND RAMONT_LIBS "${CJSON_LIBRARIES}")
    list(APPEND RAMONT_FLAGS "${CJSON_FLAGS}")
    list(APPEND RAMONT_INCLUDE_DIRS "${CJSON_INCLUDE_DIRS}")

    pkg_check_modules(LIBUV REQUIRED libuv)
    list(APPEND RAMONT_LIBS "${LIBUV_LIBRARIES}")
    list(APPEND RAMONT_FLAGS "${LIBUV_FLAGS}")
    list(APPEND RAMONT_INCLUDE_DIRS "${LIBUV_INCLUDE_DIRS}")

    include_directories(uv_socket PRIVATE "${RAMONT_INCLUDE_DIRS}")
    target_link_libraries(uv_socket PUBLIC "${RAMONT_LIBS}")
    target_compile_options(uv_socket PRIVATE "${RAMONT_FLAGS}")


#add_executable(input-socket src/queue_test.c src/queue.h)
elseif(TARGET_GROUP STREQUAL test)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
else()
    message(FATAL_ERROR "can build either release or test")
endif()

