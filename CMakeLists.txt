PROJECT (MHDShellExtensions)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_LIBRARY(libparse
  lib/libparse.h
  lib/mhd.h lib/mhd.cpp
  lib/nrrd.h lib/nrrd.cpp
  )

INCLUDE_DIRECTORIES(lib)
ADD_EXECUTABLE(debug_nrrd debug_nrrd.cpp)
TARGET_LINK_LIBRARIES(debug_nrrd libparse)

OPTION(BUILD_PARSER "Build parser for debugging" OFF)
IF(BUILD_PARSER)
  ADD_EXECUTABLE(test_mhd_parser
    mhd_parser.cpp
    )
  TARGET_LINK_LIBRARIES(test_mhd_parser libparse)
ENDIF()

ADD_DEFINITIONS(-D_UNICODE)
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
