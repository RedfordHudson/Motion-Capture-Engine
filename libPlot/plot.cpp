#include "plot.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

namespace Plot {
    // Global variables
    static GLFWwindow* g_window = nullptr;
    static SensorData g_sensor_data;
    static bool g_initialized = false;
    static float g_plot_height = 300.0f;
    static bool g_auto_fit = true;
    static float g_time_window = 10.0f;  // Show 10 seconds of data by default
    
    // Plot visibility flags
    static bool show_accel = true;
    static bool show_gyro = false;
    
    // Axis limits
    static float g_x_min = 0.0f;
    static float g_x_max = 10.0f;
    static float g_accel_y_min = -2000.0f;
    static float g_accel_y_max = 2000.0f;
    static float g_gyro_y_min = -2000.0f;
    static float g_gyro_y_max = 2000.0f;
    
    // Auto-fit data range
    static float g_accel_data_min = 0.0f;
    static float g_accel_data_max = 0.0f;
    static float g_gyro_data_min = 0.0f;
    static float g_gyro_data_max = 0.0f;
    static bool g_accel_auto_fit = true;
    static bool g_gyro_auto_fit = true;

    bool initialize(const std::string& title) {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        g_window = glfwCreateWindow(1600, 900, title.c_str(), nullptr, nullptr);
        if (!g_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(g_window);
        glfwSwapInterval(1); // Enable vsync

        // Setup ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        ImGui::StyleColorsDark();
        
        ImGui_ImplGlfw_InitForOpenGL(g_window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");

        // Initialize data
        g_sensor_data.times.reserve(MAX_POINTS);
        g_sensor_data.ax_data.reserve(MAX_POINTS);
        g_sensor_data.ay_data.reserve(MAX_POINTS);
        g_sensor_data.az_data.reserve(MAX_POINTS);
        g_sensor_data.gx_data.reserve(MAX_POINTS);
        g_sensor_data.gy_data.reserve(MAX_POINTS);
        g_sensor_data.gz_data.reserve(MAX_POINTS);

        g_initialized = true;
        return true;
    }

    void shutdown() {
        if (!g_initialized) return;
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(g_window);
        glfwTerminate();
        
        g_initialized = false;
    }

    void configurePlots(bool show_accelerometer, bool show_gyroscope) {
        show_accel = show_accelerometer;
        show_gyro = show_gyroscope;
    }

    void addDataPoint(const std::unordered_map<std::string, int>& sensor_data) {
        if (!g_initialized) return;
        
        static float start_time = -1.0f;
        float current_time = static_cast<float>(glfwGetTime());
        
        if (start_time < 0.0f) {
            start_time = current_time;
        }
        
        float time = current_time - start_time;
        
        std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
        
        // Add time
        g_sensor_data.times.push_back(time);
        
        // Add accelerometer data
        g_sensor_data.ax_data.push_back(static_cast<float>(sensor_data.count("ax") ? sensor_data.at("ax") : 0));
        g_sensor_data.ay_data.push_back(static_cast<float>(sensor_data.count("ay") ? sensor_data.at("ay") : 0));
        g_sensor_data.az_data.push_back(static_cast<float>(sensor_data.count("az") ? sensor_data.at("az") : 0));
        
        // Add gyroscope data
        g_sensor_data.gx_data.push_back(static_cast<float>(sensor_data.count("gx") ? sensor_data.at("gx") : 0));
        g_sensor_data.gy_data.push_back(static_cast<float>(sensor_data.count("gy") ? sensor_data.at("gy") : 0));
        g_sensor_data.gz_data.push_back(static_cast<float>(sensor_data.count("gz") ? sensor_data.at("gz") : 0));
        
        // Limit the number of data points
        if (g_sensor_data.times.size() > MAX_POINTS) {
            g_sensor_data.times.erase(g_sensor_data.times.begin());
            g_sensor_data.ax_data.erase(g_sensor_data.ax_data.begin());
            g_sensor_data.ay_data.erase(g_sensor_data.ay_data.begin());
            g_sensor_data.az_data.erase(g_sensor_data.az_data.begin());
            g_sensor_data.gx_data.erase(g_sensor_data.gx_data.begin());
            g_sensor_data.gy_data.erase(g_sensor_data.gy_data.begin());
            g_sensor_data.gz_data.erase(g_sensor_data.gz_data.begin());
        }
        
        // Update data range for auto-fitting
        if (g_sensor_data.times.size() > 0) {
            // Update accelerometer data range
            float accel_min = std::min({
                *std::min_element(g_sensor_data.ax_data.begin(), g_sensor_data.ax_data.end()),
                *std::min_element(g_sensor_data.ay_data.begin(), g_sensor_data.ay_data.end()),
                *std::min_element(g_sensor_data.az_data.begin(), g_sensor_data.az_data.end())
            });
            
            float accel_max = std::max({
                *std::max_element(g_sensor_data.ax_data.begin(), g_sensor_data.ax_data.end()),
                *std::max_element(g_sensor_data.ay_data.begin(), g_sensor_data.ay_data.end()),
                *std::max_element(g_sensor_data.az_data.begin(), g_sensor_data.az_data.end())
            });
            
            g_accel_data_min = accel_min;
            g_accel_data_max = accel_max;
            
            // Update gyroscope data range
            float gyro_min = std::min({
                *std::min_element(g_sensor_data.gx_data.begin(), g_sensor_data.gx_data.end()),
                *std::min_element(g_sensor_data.gy_data.begin(), g_sensor_data.gy_data.end()),
                *std::min_element(g_sensor_data.gz_data.begin(), g_sensor_data.gz_data.end())
            });
            
            float gyro_max = std::max({
                *std::max_element(g_sensor_data.gx_data.begin(), g_sensor_data.gx_data.end()),
                *std::max_element(g_sensor_data.gy_data.begin(), g_sensor_data.gy_data.end()),
                *std::max_element(g_sensor_data.gz_data.begin(), g_sensor_data.gz_data.end())
            });
            
            g_gyro_data_min = gyro_min;
            g_gyro_data_max = gyro_max;
        }
    }

    void drawPlots() {
        // Calculate window dimensions
        ImVec2 window_pos = ImGui::GetCursorScreenPos();
        ImVec2 window_size = ImGui::GetContentRegionAvail();
        float window_width = window_size.x;
        
        // Calculate how many plots are enabled
        int enabled_plots = 0;
        if (show_accel) enabled_plots++;
        if (show_gyro) enabled_plots++;
        
        // If no plots are enabled, don't draw anything
        if (enabled_plots == 0) return;
        
        // Set the time range for the sliding window
        float latest_time = 0.0f;
        {
            std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
            if (!g_sensor_data.times.empty()) {
                latest_time = g_sensor_data.times.back();
                g_x_min = latest_time - g_time_window;
                g_x_max = latest_time;
            }
        }
        
        // Update Y axis limits based on auto-fit settings
        if (g_accel_auto_fit) {
            float padding = (g_accel_data_max - g_accel_data_min) * 0.1f + 0.1f;
            g_accel_y_min = g_accel_data_min - padding;
            g_accel_y_max = g_accel_data_max + padding;
        }
        
        if (g_gyro_auto_fit) {
            float padding = (g_gyro_data_max - g_gyro_data_min) * 0.1f + 0.1f;
            g_gyro_y_min = g_gyro_data_min - padding;
            g_gyro_y_max = g_gyro_data_max + padding;
        }
        
        // Draw accelerometer plot
        if (show_accel) {
            if (ImPlot::BeginPlot("Accelerometer", ImVec2(window_width, g_plot_height), ImPlotFlags_NoTitle)) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, "Acceleration", ImPlotAxisFlags_None);
                ImPlot::SetupAxisLimits(ImAxis_X1, g_x_min, g_x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_accel_y_min, g_accel_y_max, ImGuiCond_Always);
                
                std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
                if (!g_sensor_data.times.empty()) {
                    ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("X", g_sensor_data.times.data(), g_sensor_data.ax_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Y", g_sensor_data.times.data(), g_sensor_data.ay_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Z", g_sensor_data.times.data(), g_sensor_data.az_data.data(), g_sensor_data.times.size());
                }
                ImPlot::EndPlot();
            }
        }
        
        // Draw gyroscope plot
        if (show_gyro) {
            if (ImPlot::BeginPlot("Gyroscope", ImVec2(window_width, g_plot_height), ImPlotFlags_NoTitle)) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, "Angular Velocity", ImPlotAxisFlags_None);
                ImPlot::SetupAxisLimits(ImAxis_X1, g_x_min, g_x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_gyro_y_min, g_gyro_y_max, ImGuiCond_Always);
                
                std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
                if (!g_sensor_data.times.empty()) {
                    ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("X", g_sensor_data.times.data(), g_sensor_data.gx_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.5f, 1.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Y", g_sensor_data.times.data(), g_sensor_data.gy_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Z", g_sensor_data.times.data(), g_sensor_data.gz_data.data(), g_sensor_data.times.size());
                }
                ImPlot::EndPlot();
            }
        }
    }

    void drawControls() {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Plot Settings")) {
            ImGui::SliderFloat("Plot Height", &g_plot_height, 100.0f, 500.0f, "%.0f");
            ImGui::SliderFloat("Time Window (s)", &g_time_window, 1.0f, 60.0f, "%.1f");
            
            ImGui::Separator();
            ImGui::Text("Plot Visibility:");
            ImGui::Checkbox("Accelerometer", &show_accel);
            ImGui::Checkbox("Gyroscope", &show_gyro);
            
            ImGui::Separator();
            
            if (ImGui::TreeNode("Accelerometer Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_accel_auto_fit);
                if (!g_accel_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_accel_y_min, -10000.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_accel_y_max, 0.0f, 10000.0f);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Gyroscope Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_gyro_auto_fit);
                if (!g_gyro_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_gyro_y_min, -10000.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_gyro_y_max, 0.0f, 10000.0f);
                }
                ImGui::TreePop();
            }
        }
    }

    bool renderFrame() {
        if (!g_initialized || glfwWindowShouldClose(g_window)) {
            return false;
        }
        
        // Poll events and handle
        glfwPollEvents();
        
        // Start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Create main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Main Window", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_MenuBar);
        
        // Draw the controls
        drawControls();
        
        // Draw the plots
        drawPlots();
        
        ImGui::End();
        
        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(g_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers
        glfwSwapBuffers(g_window);
        
        return true;
    }

    bool isWindowOpen() {
        return g_initialized && !glfwWindowShouldClose(g_window);
    }
} // namespace Plot 