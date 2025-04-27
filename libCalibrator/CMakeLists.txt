add_library(CalibratorLib)
target_sources(CalibratorLib 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/calibration.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/calibration.h"
)
target_include_directories(CalibratorLib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")