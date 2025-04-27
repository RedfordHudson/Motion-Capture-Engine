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
    
    // Colors for each dimension
    static const ImVec4 g_colors[] = {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red - ax
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green - ay
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue - az
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange - gx
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow - gy
        ImVec4(1.0f, 0.0f, 1.0f, 1.0f)   // Purple - gz
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
        g_window = glfwCreateWindow(800, 600, title.c_str(), NULL, NULL);
        if (g_window == NULL)
            return false;
            
        glfwMakeContextCurrent(g_window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
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
        g_sensorData.gx_data.reserve(MAX_POINTS);
        g_sensorData.gy_data.reserve(MAX_POINTS);
        g_sensorData.gz_data.reserve(MAX_POINTS);
        
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
        
        // Add data points
        g_sensorData.times.push_back(g_time);
        g_sensorData.ax_data.push_back(static_cast<float>(data_point.at("ax")));
        g_sensorData.ay_data.push_back(static_cast<float>(data_point.at("ay")));
        g_sensorData.az_data.push_back(static_cast<float>(data_point.at("az")));
        g_sensorData.gx_data.push_back(static_cast<float>(data_point.at("gx")));
        g_sensorData.gy_data.push_back(static_cast<float>(data_point.at("gy")));
        g_sensorData.gz_data.push_back(static_cast<float>(data_point.at("gz")));
        
        // Limit history size
        if (g_sensorData.times.size() > MAX_POINTS) {
            g_sensorData.times.erase(g_sensorData.times.begin());
            g_sensorData.ax_data.erase(g_sensorData.ax_data.begin());
            g_sensorData.ay_data.erase(g_sensorData.ay_data.begin());
            g_sensorData.az_data.erase(g_sensorData.az_data.begin());
            g_sensorData.gx_data.erase(g_sensorData.gx_data.begin());
            g_sensorData.gy_data.erase(g_sensorData.gy_data.begin());
            g_sensorData.gz_data.erase(g_sensorData.gz_data.begin());
        }
    }

    void drawPlots() {
        if (!g_initialized)
            return;
            
        std::lock_guard<std::mutex> lock(g_sensorData.mtx);
        
        ImGui::Begin("Motion Sensor Data");
        
        if (ImPlot::BeginPlot("Accelerometer Data", ImVec2(-1, 300))) {
            ImPlot::SetupAxes("Time (s)", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            
            if (!g_sensorData.times.empty()) {
                ImPlot::SetNextLineStyle(g_colors[0]);
                ImPlot::PlotLine("ax", g_sensorData.times.data(), g_sensorData.ax_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
                
                ImPlot::SetNextLineStyle(g_colors[1]);
                ImPlot::PlotLine("ay", g_sensorData.times.data(), g_sensorData.ay_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
                
                ImPlot::SetNextLineStyle(g_colors[2]);
                ImPlot::PlotLine("az", g_sensorData.times.data(), g_sensorData.az_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
            }
            
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Gyroscope Data", ImVec2(-1, 300))) {
            ImPlot::SetupAxes("Time (s)", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            
            if (!g_sensorData.times.empty()) {
                ImPlot::SetNextLineStyle(g_colors[3]);
                ImPlot::PlotLine("gx", g_sensorData.times.data(), g_sensorData.gx_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
                
                ImPlot::SetNextLineStyle(g_colors[4]);
                ImPlot::PlotLine("gy", g_sensorData.times.data(), g_sensorData.gy_data.data(), 
                                static_cast<int>(g_sensorData.times.size()));
                
                ImPlot::SetNextLineStyle(g_colors[5]);
                ImPlot::PlotLine("gz", g_sensorData.times.data(), g_sensorData.gz_data.data(), 
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