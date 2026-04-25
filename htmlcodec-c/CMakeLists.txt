# CMakeList.txt : CMake project for htmlcodec-c, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<C_COMPILER_ID:MSVC>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("htmlcodec-c" C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

# --- zlib (fetched from official source) ---
include(FetchContent)

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    zlib
    URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(zlib)

# Core tokenizer library target (static)
add_library(htmlcodec-c-lib STATIC
    html-tokenizer.c
    css-tokenizer.c
    css-codec.c
    nl-en-tokenizer.c
    nl-en-codec.c
    cl-javascript-en-tokenizer.c
    nl-en-us-hyphenator.c
)

target_include_directories(htmlcodec-c-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
target_link_libraries(htmlcodec-c-test PRIVATE htmlcodec-c-lib zlibstatic)
target_include_directories(htmlcodec-c-test PRIVATE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})

add_test(NAME htmlcodec-c-test COMMAND htmlcodec-c-test)

