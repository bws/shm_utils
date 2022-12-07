enable_testing()

include_directories(
  ${gtest_SOURCE_DIR}/include 
  ${gtest_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)

add_executable(shm_test
  ${CMAKE_CURRENT_SOURCE_DIR}/shm_vector_test.cc
)

target_sources(shm_test PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/shm_list_test.cc
${CMAKE_CURRENT_SOURCE_DIR}/shm_vector_test.cc
)

target_link_libraries(
  shm_test
  shmvector
  rt
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(shm_test)

