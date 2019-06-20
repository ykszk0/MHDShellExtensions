PROJECT (MHDShellExtensionsTest)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

add_executable(normal normal.cpp)
target_link_libraries(normal libparse)
add_test(
  NAME NormalTest
  COMMAND normal ${CMAKE_CURRENT_LIST_DIR}/file/normal/normal.mhd
  )

add_executable(exception exception.cpp)
target_link_libraries(exception libparse)
add_test(
  NAME ExceptionTest
  COMMAND exception ${CMAKE_CURRENT_LIST_DIR}/file/exception/empty.mhd
  )

add_executable(print_header print_header.cpp)
target_link_libraries(print_header libparse)
