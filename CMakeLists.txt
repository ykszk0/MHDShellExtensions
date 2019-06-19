PROJECT (MHDShellExtensions)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_LIBRARY(libmhd
libmhd.h libmhd.cpp
)
ADD_LIBRARY(libnrrd
libnrrd.h libnrrd.cpp
)
ADD_EXECUTABLE(debug_nrrd debug_nrrd.cpp)
TARGET_LINK_LIBRARIES(debug_nrrd libnrrd)

OPTION(BUILD_PARSER "Build parser for debugging" OFF)
IF(BUILD_PARSER)
  ADD_EXECUTABLE(test_mhd_parser
    mhd_parser.cpp
    )
  TARGET_LINK_LIBRARIES(test_mhd_parser libmhd)
ENDIF()

ADD_DEFINITIONS(-D_UNICODE)
ADD_LIBRARY(MHDInfotip SHARED
  MHDInfotip.h MHDInfotip.cpp MHDInfotip.def
  )
TARGET_LINK_LIBRARIES(MHDInfotip libmhd)

ADD_LIBRARY(MHDPreviewHandler SHARED
  MHDPreviewHandler.h MHDPreviewHandler.cpp MHDPreviewHandler.def
  )
TARGET_LINK_LIBRARIES(MHDPreviewHandler libmhd)

ADD_LIBRARY(MHDIconHandler SHARED
  MHDIconHandler.h MHDIconHandler.cpp
  MHDIconHandler.def res/icons.rc
  )
TARGET_LINK_LIBRARIES(MHDIconHandler libmhd)

ADD_LIBRARY(MHDPropertyStore SHARED
  MHDPropertyStore.h MHDPropertyStore.cpp
  MHDPropertyStore.def
  )
TARGET_LINK_LIBRARIES(MHDPropertyStore libmhd)
