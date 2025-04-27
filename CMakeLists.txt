cmake_minimum_required(VERSION 3.10)

project(MotionCaptureEngine)

add_executable(main main.cpp)

add_subdirectory(libSerial)

target_link_libraries(main PRIVATE SerialMonitor)
