cmake_minimum_required(VERSION 3.17)
project(udp_server_connections C)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else ()
    message("no debug")
    set(isDebug FALSE)
endif ()

if (isDebug)
    message("Debug build detected")
    add_compile_definitions(CONFIGURATION_DEBUG TYRAN_CONFIGURATION_DEBUG)
    set(CONFIGURATION_DEBUG 1)
endif ()

set(DEPS ../../deps/)

file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS "${DEPS}*/*/src/lib/*.c")

add_executable(
        udp_server_connections_example
        main.c ${deps_src}
)

set_property(TARGET udp_server_connections_example PROPERTY C_STANDARD 11)

if (WIN32)
    target_link_libraries(udp_server_connections_example udp_server_connections)
else ()
    target_link_libraries(udp_server_connections_example PUBLIC udp_server_connections m)
endif (WIN32)

target_compile_options(
        udp_server_connections_example
        PRIVATE -Wall
        -Wextra
        -Wshadow
        -Wstrict-aliasing
        -ansi
        -pedantic
        -Wno-declaration-after-statement
        -Wno-extra-semi-stmt
        -Wno-undef
        -Wno-unused-variable
        -Wno-unused-parameter
        -Wno-padded
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
        -Werror=missing-prototypes
        -Werror=int-conversion
        -Werror=return-type
        -Werror=incompatible-function-pointer-types)

if (COMPILER_CLANG)
    target_compile_options(
            udp_server_connections_example PRIVATE -Wmost -Weverything
            -Werror=missing-variable-declarations)
endif ()

