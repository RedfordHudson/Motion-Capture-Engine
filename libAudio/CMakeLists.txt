add_library(AudioLib)
target_sources(AudioLib 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Audio.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Audio.h"
)
target_include_directories(AudioLib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

# Link Windows multimedia library
if(WIN32)
  target_link_libraries(AudioLib PRIVATE winmm)
endif()