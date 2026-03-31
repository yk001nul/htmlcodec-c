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

# Core tokenizer library target (static)
add_library(htmlcodec-c-lib STATIC
    html-tokenizer.c
    css-tokenizer.c
    nl-en-tokenizer.c
    cl-javascript-en-tokenizer.c
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
target_link_libraries(htmlcodec-c-test PRIVATE htmlcodec-c-lib)

add_test(NAME htmlcodec-c-test COMMAND htmlcodec-c-test)

