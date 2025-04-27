cmake_minimum_required(VERSION 3.10)

project(MotionCaptureEngine)

# C++17 is required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the subdirectories
add_subdirectory(libSerial)
add_subdirectory(libPlot)
add_subdirectory(libHand)
add_subdirectory(libAudio)
add_subdirectory(libCalibrator)
add_subdirectory(libUncoupler)

# Create the main executable
add_executable(main main.cpp)

# Link against the libraries
target_link_libraries(main SerialMonitor PlotLib HandLib AudioLib CalibrationLib Uncoupler)

# Copy assets to build directory (with specific mention of WAV files)
file(GLOB ASSET_FILES 
     "${CMAKE_SOURCE_DIR}/assets/*.mp3"
     "${CMAKE_SOURCE_DIR}/assets/*.wav"
)

# Copy each asset file individually to ensure they're properly handled
foreach(ASSET_FILE ${ASSET_FILES})
    get_filename_component(FILENAME ${ASSET_FILE} NAME)
    add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets
        COMMAND ${CMAKE_COMMAND} -E copy ${ASSET_FILE} ${CMAKE_BINARY_DIR}/assets/${FILENAME}
        COMMENT "Copying asset file ${FILENAME}"
    )
endforeach()
