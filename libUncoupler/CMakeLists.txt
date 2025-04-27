add_library(Uncoupler)
target_sources(Uncoupler 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/uncoupler.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/uncoupler.h"
)
target_include_directories(Uncoupler PUBLIC "${CMAKE_CURRENT_LIST_DIR}")