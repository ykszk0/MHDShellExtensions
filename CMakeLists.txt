PROJECT (MHDShellExtensions)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_LIBRARY(libmhd
libmhd.h libmhd.cpp
)

# ADD_EXECUTABLE(test_mhd_parser
#   mhd_parser.cpp
#   )
# target_link_libraries(test_mhd_parser libmhd)

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
