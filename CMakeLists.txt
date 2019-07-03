PROJECT (MHDShellExtensions)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
ADD_DEFINITIONS(-D_UNICODE)

add_subdirectory(zlib)
INCLUDE_DIRECTORIES(zlib)
ADD_LIBRARY(libparse
  lib/libparse.h lib/libparse.cpp
  lib/mhd.h lib/mhd.cpp
  lib/nrrd.h lib/nrrd.cpp
  lib/nifti.h lib/nifti.cpp
  )
target_link_libraries(libparse zlibstatic)

INCLUDE_DIRECTORIES(lib)

OPTION(BUILD_TEST "Enable testing" OFF)
IF(BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
ENDIF()

ADD_LIBRARY(MHDInfotip SHARED
  MHDInfotip.h MHDInfotip.cpp MHDInfotip.def
  )
TARGET_LINK_LIBRARIES(MHDInfotip libparse)

ADD_LIBRARY(MHDPreviewHandler SHARED
  MHDPreviewHandler.h MHDPreviewHandler.cpp MHDPreviewHandler.def
  )
TARGET_LINK_LIBRARIES(MHDPreviewHandler libparse)

ADD_LIBRARY(MHDIconHandler SHARED
  MHDIconHandler.h MHDIconHandler.cpp
  MHDIconHandler.def res/icons.rc
  )
TARGET_LINK_LIBRARIES(MHDIconHandler libparse)

ADD_LIBRARY(MHDPropertyStore SHARED
  MHDPropertyStore.h MHDPropertyStore.cpp
  MHDPropertyStore.def
  )
TARGET_LINK_LIBRARIES(MHDPropertyStore libparse)
