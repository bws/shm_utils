
#target_sources(shmarray PRIVATE
#  ${CMAKE_CURRENT_SOURCE_DIR}/shmarray.c
#)

add_library(shmvector STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/shm_mutex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shm_mutex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/shm_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shm_vector.c
)

