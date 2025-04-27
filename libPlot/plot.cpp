#include "plot.h"
#include <chrono>
#include <iostream>
#include <algorithm>

// ImGui and ImPlot headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>

namespace Plot {
    // Global state
    static SensorData g_sensorData;
    static GLFWwindow* g_window = nullptr;
    static float g_time = 0.0f;
    static bool g_initialized = false;
    
    // Plot settings
    static float g_plot_height = 200.0f;
    static float g_plot_width = -1.0f; // Full width
    static float g_time_window = 5.0f; // Show 5 seconds of data
    static bool g_auto_fit_y = true;  // Auto-fit only the Y axis
    static float g_y_min = -2000.0f;  // Increased range for Y min
    static float g_y_max = 2000.0f;   // Increased range for Y max
    
    // Auto detection of data range
    static float g_data_min = -2000.0f;
    static float g_data_max = 2000.0f;
    static bool g_auto_detect_range = true;
    
    // Colors for each dimension
    static const ImVec4 g_colors[] = {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red - ax
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green - ay
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue - az
    };

    // Error callback for GLFW
    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    }

    bool initialize(const std::string& title) {
        // Set error callback
        glfwSetErrorCallback(glfw_error_callback);
        
        // Initialize GLFW
        if (!glfwInit())
            return false;

        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        // Create window with graphics context
        g_window = glfwCreateWindow(1280, 720, title.c_str(), NULL, NULL);
        if (g_window == NULL)
            return false;
            
        glfwMakeContextCurrent(g_window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(g_window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Setup style
        ImGui::StyleColorsDark();
        
        // Reserve space in vectors to avoid reallocations
        g_sensorData.times.reserve(MAX_POINTS);
        g_sensorData.ax_data.reserve(MAX_POINTS);
        g_sensorData.ay_data.reserve(MAX_POINTS);
        g_sensorData.az_data.reserve(MAX_POINTS);
        
        g_initialized = true;
        return true;
    }

    void shutdown() {
        if (!g_initialized)
            return;
            
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow(g_window);
        glfwTerminate();
        
        g_initialized = false;
    }

    void addDataPoint(const std::unordered_map<std::string, int>& data_point) {
        if (!g_initialized)
            return;
            
        std::lock_guard<std::mutex> lock(g_sensorData.mtx);
        
        // Update time
        g_time += 0.01f; // Assume 100Hz sampling rate
        
        // Add data points for acceleration
        g_sensorData.times.push_back(g_time);
        float ax = static_cast<float>(data_point.at("ax"));
        float ay = static_cast<float>(data_point.at("ay"));
        float az = static_cast<float>(data_point.at("az"));
        
        g_sensorData.ax_data.push_back(ax);
        g_sensorData.ay_data.push_back(ay);
        g_sensorData.az_data.push_back(az);
        
        // Auto-detect data range
        if (g_auto_detect_range) {
            // Update min/max values to ensure all data fits
            g_data_min = std::min(g_data_min, std::min(ax, std::min(ay, az)));
            g_data_max = std::max(g_data_max, std::max(ax, std::max(ay, az)));
            
            // Add 10% padding
            float range = g_data_max - g_data_min;
            float padding = range * 0.1f;
            
            // Update y-axis limits if using auto-fit
            if (g_auto_fit_y) {
                g_y_min = g_data_min - padding;
                g_y_max = g_data_max + padding;
            }
        }
        
        // Limit history size
        if (g_sensorData.times.size() > MAX_POINTS) {
            g_sensorData.times.erase(g_sensorData.times.begin());
            g_sensorData.ax_data.erase(g_sensorData.ax_data.begin());
            g_sensorData.ay_data.erase(g_sensorData.ay_data.begin());
            g_sensorData.az_data.erase(g_sensorData.az_data.begin());
        }
    }

    // Internal function to draw the plots
    static void drawPlots() {
        if (!g_initialized)
            return;
            
        std::lock_guard<std::mutex> lock(g_sensorData.mtx);
        
        // Set window size to maximize space
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        ImGui::Begin("Accelerometer Data", nullptr, 
                    ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoCollapse);
        
        // Plot controls
        ImGui::SliderFloat("Plot Height", &g_plot_height, 100.0f, 400.0f, "%.0f px");
        ImGui::SliderFloat("Time Window (s)", &g_time_window, 1.0f, 20.0f, "%.1f s");
        
        // Y-axis controls
        if (ImGui::Checkbox("Auto-fit Y-Axis", &g_auto_fit_y)) {
            if (g_auto_fit_y) {
                // Reset detection when switching to auto-fit
                g_auto_detect_range = true;
            }
        }
        
        if (!g_auto_fit_y) {
            // Wider range for manual Y-axis limits
            ImGui::SliderFloat("Y Min", &g_y_min, -20000.0f, 0.0f);
            ImGui::SliderFloat("Y Max", &g_y_max, 0.0f, 20000.0f);
        } else {
            // Display detected range
            ImGui::Text("Detected Range: %.0f to %.0f", g_data_min, g_data_max);
            
            // Button to reset range detection
            if (ImGui::Button("Reset Range Detection")) {
                g_data_min = 0.0f;
                g_data_max = 0.0f;
                g_auto_detect_range = true;
            }
        }
        
        // Display current values
        if (!g_sensorData.times.empty()) {
            ImGui::Text("Current Time: %.2f", g_time);
            ImGui::Text("Acceleration: X=%.1f, Y=%.1f, Z=%.1f", 
                g_sensorData.ax_data.back(),
                g_sensorData.ay_data.back(),
                g_sensorData.az_data.back());
        }
        
        ImGui::Separator();
        
        // Set x-axis limits for sliding window
        float x_min = g_time - g_time_window;
        float x_max = g_time;
        
        // Set the axis limits
        ImPlotAxisFlags x_flags = 0; // No auto-fit for x-axis
        ImPlotAxisFlags y_flags = 0; // We'll handle y-axis limits manually
        
        // Set fixed axis limits
        ImPlot::SetNextAxesLimits(x_min, x_max, g_y_min, g_y_max, ImGuiCond_Always);
        
        // Accelerometer data plot
        if (ImPlot::BeginPlot("Accelerometer Data", ImVec2(g_plot_width, g_plot_height))) {
            ImPlot::SetupAxes("Time (s)", "Raw Acceleration", x_flags, y_flags);
            
            if (!g_sensorData.times.empty()) {
                ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                ImPlot::PlotLine("X-axis", g_sensorData.times.data(), g_sensorData.ax_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
                
                ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                ImPlot::PlotLine("Y-axis", g_sensorData.times.data(), g_sensorData.ay_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
                
                ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                ImPlot::PlotLine("Z-axis", g_sensorData.times.data(), g_sensorData.az_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
            }
            
            ImPlot::EndPlot();
        }
        
        ImGui::End();
    }

    bool renderFrame() {
        if (!g_initialized || glfwWindowShouldClose(g_window))
            return false;
            
        // Poll and handle events
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw plots
        drawPlots();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(g_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_window);
        
        return true;
    }

    bool isWindowOpen() {
        return g_initialized && !glfwWindowShouldClose(g_window);
    }
} 