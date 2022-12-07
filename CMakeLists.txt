cmake_minimum_required(VERSION 3.14)
project(SHM_data_structures)

# Add source directories
add_subdirectory(src)
add_subdirectory(test)

#
# Setup Google GTest framework
#
# GoogleTest requires at least C++14
#
set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


