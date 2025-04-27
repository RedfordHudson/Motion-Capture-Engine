# Add library
add_library(HandLib)

# Set source files
target_sources(HandLib 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/hand.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/hand.h"
)

# Include directories
target_include_directories(HandLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}") 