add_library(CalibrationLib)
target_sources(CalibrationLib 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/calibration.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/calibration.h"
)
target_include_directories(CalibrationLib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")