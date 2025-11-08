cmake_minimum_required(VERSION 3.13)
project(phigros_chart_merge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten.cmake")
    set(CMAKE_C_FLAGS "-std=gnu11 -Oz -g0 --profiling-funcs -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS "-std=gnu++17 -Oz -g0 -fno-exceptions -fno-rtti")
endif()

# 头文件目录
include_directories(include)

# 源文件
add_executable(json_parser src/json_parser.cpp src/miniz.c)

# WebAssembly 链接选项
if (CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten.cmake")
    set_target_properties(json_parser PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/public"
        LINK_FLAGS "--bind \
            -s  \"EXPORTED_FUNCTIONS=[ \
                \\\"_parse_json\\\", \\\"_extract_pez\\\", \
                \\\"_init_merge\\\", \\\"_process_merge_chunk\\\", \
                 \\\"_finalize_merge\\\", \
                \\\"_malloc\\\", \\\"_free\\\"]\" \
            -s  \"EXPORTED_RUNTIME_METHODS=[ \
                \\\"lengthBytesUTF8\\\", \\\"stringToUTF8\\\", \
                \\\"HEAPU8\\\", \\\"UTF8ToString\\\"]\" \
            -s  \"ASSERTIONS=0\" \
            -s  \"MALLOC=dlmalloc\" \
            -s  \"ALLOW_MEMORY_GROWTH=1\" \
            -s  \"INITIAL_MEMORY=134217728\" \
            -s  \"MAXIMUM_MEMORY=2147483648\" \
            -s  \"NO_FILESYSTEM=1\" \
            -s  \"NODEJS_CATCH_EXIT=0\" \
            -s  \"NODEJS_CATCH_REJECTION=0\""
    )
endif()