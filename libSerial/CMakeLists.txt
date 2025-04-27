add_library(SerialMonitor)
target_sources(SerialMonitor 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/serial.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/serial.h"
)
target_include_directories(SerialMonitor PUBLIC "${CMAKE_CURRENT_LIST_DIR}")