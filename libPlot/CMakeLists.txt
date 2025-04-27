# Add library
add_library(PlotLib)

# Set source files
target_sources(PlotLib 
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/plot.cpp"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/plot.h"
)

# Include directories
target_include_directories(PlotLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Find required packages
find_package(OpenGL REQUIRED)

# Download and build ImGui and ImPlot
include(FetchContent)

# ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.89.9
)

# ImPlot
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v0.16
)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)

FetchContent_MakeAvailable(imgui implot glfw)

# Add ImGui source files to our target
target_sources(PlotLib PRIVATE
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)

# Include directories for dependencies
target_include_directories(PlotLib PRIVATE
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${implot_SOURCE_DIR}
  ${glfw_SOURCE_DIR}/include
)

# Link against OpenGL and GLFW
target_link_libraries(PlotLib PRIVATE OpenGL::GL glfw) 