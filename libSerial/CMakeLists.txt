add_library(SerialMonitor)
target_sources(SerialMonitor PRIVATE serial.cpp PUBLIC serial.h)
target_include_directories(SerialMonitor PUBLIC ${CMAKE_CURRENT_LIST_DIR})