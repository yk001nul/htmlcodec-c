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

# Add source to this project's executable.
add_executable (htmlcodec-c "htmlcodec-c.c" "html-tokenizer.c" "css-tokenizer.c" "nl-en-tokenizer.c" "cl-javascript-en-tokenizer.c" "tokenizer-test.c")

# TODO: Add tests and install targets if needed.
