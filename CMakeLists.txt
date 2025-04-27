cmake_minimum_required(VERSION 3.10)

project(MotionCaptureEngine)

# C++17 is required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the subdirectories
add_subdirectory(libSerial)
add_subdirectory(libPlot)
add_subdirectory(libHand)

# Create the main executable
add_executable(main main.cpp)

# Link against the libraries
target_link_libraries(main SerialMonitor PlotLib HandLib)
