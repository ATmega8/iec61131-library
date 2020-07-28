cmake_minimum_required(VERSION 3.15)
project(iec61131_library)

set(CMAKE_CXX_STANDARD 14)

include_directories(inc)

add_executable(iec61131_library
        src/glueVars.c
        src/lua_value_operation.c
        src/lua_bit_array.c
        src/lua_timer.c
        src/main.c)

add_library(operation_value SHARED src/lua_value_operation.c)

target_link_libraries(iec61131_library lua)
