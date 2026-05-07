# CMakeList.txt : CMake project for htmlcodec-c, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.21)

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<C_COMPILER_ID:MSVC>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("htmlcodec-c" C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

# --- zlib: prefer system library; fall back to FetchContent when not found ---
# macOS ships zlib in the SDK; Linux typically has it via libz-dev; Android NDK
# includes it natively.  Windows and other bare environments use FetchContent.
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    set(HTMLCODEC_ZLIB_TARGET ZLIB::ZLIB)
    set(HTMLCODEC_ZLIB_INCLUDES "")
else()
    include(FetchContent)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        zlib
        URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(zlib)
    set(HTMLCODEC_ZLIB_TARGET zlibstatic)
    set(HTMLCODEC_ZLIB_INCLUDES ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
endif()

# Core tokenizer library target (static)
add_library(htmlcodec-c-lib STATIC
    html-tokenizer.c
    html-codec.c
    css-tokenizer.c
    css-codec.c
    nl-en-tokenizer.c
    nl-en-opt.c
    nl-en-codec.c
    cl-javascript-en-tokenizer.c
    cl-javascript-codec.c
    nl-en-us-hyphenator.c
)

target_include_directories(htmlcodec-c-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Platform-specific compiler diagnostics
if(MSVC)
    target_compile_options(htmlcodec-c-lib PRIVATE /W3)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(htmlcodec-c-lib PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
endif()

# Main runtime executable (non-test)
add_executable(htmlcodec-c
    htmlcodec-c.c
)
target_link_libraries(htmlcodec-c PRIVATE htmlcodec-c-lib)

# Test executable
add_executable(htmlcodec-c-test
    test-runner.c
    tokenizer-test.c
)
target_link_libraries(htmlcodec-c-test PRIVATE htmlcodec-c-lib ${HTMLCODEC_ZLIB_TARGET})
if(HTMLCODEC_ZLIB_INCLUDES)
    target_include_directories(htmlcodec-c-test PRIVATE ${HTMLCODEC_ZLIB_INCLUDES})
endif()

add_test(NAME htmlcodec-c-test COMMAND htmlcodec-c-test)
