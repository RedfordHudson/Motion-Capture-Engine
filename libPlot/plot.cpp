#include "plot.h"
#include "../libHand/hand.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

// Forward declaration for the global tracker from main.cpp
extern Hand::HandTracker g_tracker;

namespace Plot {
    // Forward declarations
    void updateDataRanges();
    void drawPlots();
    void drawControls();

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
    static bool show_velocity = false;
    static bool show_gravity = false;
    static bool show_linear_accel = false;
    
    // Axis limits
    static float g_x_min = 0.0f;
    static float g_x_max = 10.0f;
    static float g_accel_y_min = -32768.0f;
    static float g_accel_y_max = 32767.0f;
    static float g_gyro_y_min = -32768.0f;
    static float g_gyro_y_max = 32767.0f;
    static float g_velocity_y_min = -1000.0f;
    static float g_velocity_y_max = 1000.0f;
    static float g_gravity_y_min = -10.0f;
    static float g_gravity_y_max = 10.0f;
    static float g_linear_accel_y_min = -32768.0f;
    static float g_linear_accel_y_max = 32767.0f;
    
    // Auto-fit data range
    static float g_accel_data_min = 0.0f;
    static float g_accel_data_max = 0.0f;
    static float g_gyro_data_min = 0.0f;
    static float g_gyro_data_max = 0.0f;
    static float g_velocity_data_min = 0.0f;
    static float g_velocity_data_max = 0.0f;
    static float g_gravity_data_min = -10.0f;
    static float g_gravity_data_max = 10.0f;
    static float g_linear_accel_data_min = 0.0f;
    static float g_linear_accel_data_max = 0.0f;
    static bool g_accel_auto_fit = true;
    static bool g_gyro_auto_fit = true;
    static bool g_velocity_auto_fit = true;
    static bool g_gravity_auto_fit = true;
    static bool g_linear_accel_auto_fit = true;

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
        g_sensor_data.vx_data.reserve(MAX_POINTS);
        g_sensor_data.vy_data.reserve(MAX_POINTS);
        g_sensor_data.vz_data.reserve(MAX_POINTS);
        g_sensor_data.gravity_x_data.reserve(MAX_POINTS);
        g_sensor_data.gravity_y_data.reserve(MAX_POINTS);
        g_sensor_data.gravity_z_data.reserve(MAX_POINTS);
        g_sensor_data.linear_ax_data.reserve(MAX_POINTS);
        g_sensor_data.linear_ay_data.reserve(MAX_POINTS);
        g_sensor_data.linear_az_data.reserve(MAX_POINTS);

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

    void configurePlots(bool show_accelerometer, bool show_gyroscope, bool show_velocity_plot, bool show_gravity_plot, bool show_linear_accel_plot) {
        show_accel = show_accelerometer;
        show_gyro = show_gyroscope;
        show_velocity = show_velocity_plot;
        show_gravity = show_gravity_plot;
        show_linear_accel = show_linear_accel_plot;
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
        
        // Get velocity data from the main application's tracker
        Hand::Vector3D velocity = g_tracker.getVelocity();
        g_sensor_data.vx_data.push_back(velocity.x);
        g_sensor_data.vy_data.push_back(velocity.y);
        g_sensor_data.vz_data.push_back(velocity.z);
        
        // Add placeholder data for gravity and linear acceleration
        g_sensor_data.gravity_x_data.push_back(0.0f);
        g_sensor_data.gravity_y_data.push_back(0.0f);
        g_sensor_data.gravity_z_data.push_back(0.0f);
        g_sensor_data.linear_ax_data.push_back(g_sensor_data.ax_data.back());
        g_sensor_data.linear_ay_data.push_back(g_sensor_data.ay_data.back());
        g_sensor_data.linear_az_data.push_back(g_sensor_data.az_data.back());
        
        // Limit the number of data points
        if (g_sensor_data.times.size() > MAX_POINTS) {
            g_sensor_data.times.erase(g_sensor_data.times.begin());
            g_sensor_data.ax_data.erase(g_sensor_data.ax_data.begin());
            g_sensor_data.ay_data.erase(g_sensor_data.ay_data.begin());
            g_sensor_data.az_data.erase(g_sensor_data.az_data.begin());
            g_sensor_data.gx_data.erase(g_sensor_data.gx_data.begin());
            g_sensor_data.gy_data.erase(g_sensor_data.gy_data.begin());
            g_sensor_data.gz_data.erase(g_sensor_data.gz_data.begin());
            g_sensor_data.vx_data.erase(g_sensor_data.vx_data.begin());
            g_sensor_data.vy_data.erase(g_sensor_data.vy_data.begin());
            g_sensor_data.vz_data.erase(g_sensor_data.vz_data.begin());
            g_sensor_data.gravity_x_data.erase(g_sensor_data.gravity_x_data.begin());
            g_sensor_data.gravity_y_data.erase(g_sensor_data.gravity_y_data.begin());
            g_sensor_data.gravity_z_data.erase(g_sensor_data.gravity_z_data.begin());
            g_sensor_data.linear_ax_data.erase(g_sensor_data.linear_ax_data.begin());
            g_sensor_data.linear_ay_data.erase(g_sensor_data.linear_ay_data.begin());
            g_sensor_data.linear_az_data.erase(g_sensor_data.linear_az_data.begin());
        }
        
        // Update data ranges for auto-fitting
        updateDataRanges();
    }
    
    void addDataPointWithGravity(const std::unordered_map<std::string, int>& sensor_data,
                              float gravity_x, float gravity_y, float gravity_z,
                              float linear_ax, float linear_ay, float linear_az) {
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
        
        // Get velocity data from the main application's tracker
        Hand::Vector3D velocity = g_tracker.getVelocity();
        g_sensor_data.vx_data.push_back(velocity.x);
        g_sensor_data.vy_data.push_back(velocity.y);
        g_sensor_data.vz_data.push_back(velocity.z);
        
        // Add gravity vector data
        g_sensor_data.gravity_x_data.push_back(gravity_x);
        g_sensor_data.gravity_y_data.push_back(gravity_y);
        g_sensor_data.gravity_z_data.push_back(gravity_z);
        
        // Add linear acceleration data (acceleration with gravity removed)
        g_sensor_data.linear_ax_data.push_back(linear_ax);
        g_sensor_data.linear_ay_data.push_back(linear_ay);
        g_sensor_data.linear_az_data.push_back(linear_az);
        
        // Limit the number of data points
        if (g_sensor_data.times.size() > MAX_POINTS) {
            g_sensor_data.times.erase(g_sensor_data.times.begin());
            g_sensor_data.ax_data.erase(g_sensor_data.ax_data.begin());
            g_sensor_data.ay_data.erase(g_sensor_data.ay_data.begin());
            g_sensor_data.az_data.erase(g_sensor_data.az_data.begin());
            g_sensor_data.gx_data.erase(g_sensor_data.gx_data.begin());
            g_sensor_data.gy_data.erase(g_sensor_data.gy_data.begin());
            g_sensor_data.gz_data.erase(g_sensor_data.gz_data.begin());
            g_sensor_data.vx_data.erase(g_sensor_data.vx_data.begin());
            g_sensor_data.vy_data.erase(g_sensor_data.vy_data.begin());
            g_sensor_data.vz_data.erase(g_sensor_data.vz_data.begin());
            g_sensor_data.gravity_x_data.erase(g_sensor_data.gravity_x_data.begin());
            g_sensor_data.gravity_y_data.erase(g_sensor_data.gravity_y_data.begin());
            g_sensor_data.gravity_z_data.erase(g_sensor_data.gravity_z_data.begin());
            g_sensor_data.linear_ax_data.erase(g_sensor_data.linear_ax_data.begin());
            g_sensor_data.linear_ay_data.erase(g_sensor_data.linear_ay_data.begin());
            g_sensor_data.linear_az_data.erase(g_sensor_data.linear_az_data.begin());
        }
        
        // Update data ranges for auto-fitting
        updateDataRanges();
    }
    
    // Helper to update data ranges for auto-fitting
    void updateDataRanges() {
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
            
            // Update velocity data range
            float velocity_min = std::min({
                *std::min_element(g_sensor_data.vx_data.begin(), g_sensor_data.vx_data.end()),
                *std::min_element(g_sensor_data.vy_data.begin(), g_sensor_data.vy_data.end()),
                *std::min_element(g_sensor_data.vz_data.begin(), g_sensor_data.vz_data.end())
            });
            
            float velocity_max = std::max({
                *std::max_element(g_sensor_data.vx_data.begin(), g_sensor_data.vx_data.end()),
                *std::max_element(g_sensor_data.vy_data.begin(), g_sensor_data.vy_data.end()),
                *std::max_element(g_sensor_data.vz_data.begin(), g_sensor_data.vz_data.end())
            });
            
            g_velocity_data_min = velocity_min;
            g_velocity_data_max = velocity_max;
            
            // Update gravity data range
            float gravity_min = std::min({
                *std::min_element(g_sensor_data.gravity_x_data.begin(), g_sensor_data.gravity_x_data.end()),
                *std::min_element(g_sensor_data.gravity_y_data.begin(), g_sensor_data.gravity_y_data.end()),
                *std::min_element(g_sensor_data.gravity_z_data.begin(), g_sensor_data.gravity_z_data.end())
            });
            
            float gravity_max = std::max({
                *std::max_element(g_sensor_data.gravity_x_data.begin(), g_sensor_data.gravity_x_data.end()),
                *std::max_element(g_sensor_data.gravity_y_data.begin(), g_sensor_data.gravity_y_data.end()),
                *std::max_element(g_sensor_data.gravity_z_data.begin(), g_sensor_data.gravity_z_data.end())
            });
            
            g_gravity_data_min = gravity_min;
            g_gravity_data_max = gravity_max;
            
            // Update linear acceleration data range
            float linear_accel_min = std::min({
                *std::min_element(g_sensor_data.linear_ax_data.begin(), g_sensor_data.linear_ax_data.end()),
                *std::min_element(g_sensor_data.linear_ay_data.begin(), g_sensor_data.linear_ay_data.end()),
                *std::min_element(g_sensor_data.linear_az_data.begin(), g_sensor_data.linear_az_data.end())
            });
            
            float linear_accel_max = std::max({
                *std::max_element(g_sensor_data.linear_ax_data.begin(), g_sensor_data.linear_ax_data.end()),
                *std::max_element(g_sensor_data.linear_ay_data.begin(), g_sensor_data.linear_ay_data.end()),
                *std::max_element(g_sensor_data.linear_az_data.begin(), g_sensor_data.linear_az_data.end())
            });
            
            g_linear_accel_data_min = linear_accel_min;
            g_linear_accel_data_max = linear_accel_max;
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
        if (show_velocity) enabled_plots++;
        if (show_gravity) enabled_plots++;
        if (show_linear_accel) enabled_plots++;
        
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
            // Improved padding calculation for auto-fit to handle larger ranges
            float range = g_accel_data_max - g_accel_data_min;
            float padding = range * 0.1f + 1.0f;
            g_accel_y_min = g_accel_data_min - padding;
            g_accel_y_max = g_accel_data_max + padding;
        }
        
        if (g_gyro_auto_fit) {
            // Improved padding calculation for auto-fit to handle larger ranges
            float range = g_gyro_data_max - g_gyro_data_min;
            float padding = range * 0.1f + 1.0f;
            g_gyro_y_min = g_gyro_data_min - padding;
            g_gyro_y_max = g_gyro_data_max + padding;
        }
        
        if (g_velocity_auto_fit) {
            // Improved padding calculation for auto-fit to handle larger ranges
            float range = g_velocity_data_max - g_velocity_data_min;
            float padding = range * 0.1f + 1.0f;
            g_velocity_y_min = g_velocity_data_min - padding;
            g_velocity_y_max = g_velocity_data_max + padding;
        }
        
        if (g_gravity_auto_fit) {
            // Improved padding calculation for auto-fit to handle larger ranges
            float range = g_gravity_data_max - g_gravity_data_min;
            float padding = range * 0.1f + 1.0f;
            g_gravity_y_min = g_gravity_data_min - padding;
            g_gravity_y_max = g_gravity_data_max + padding;
        }
        
        if (g_linear_accel_auto_fit) {
            // Improved padding calculation for auto-fit to handle larger ranges
            float range = g_linear_accel_data_max - g_linear_accel_data_min;
            float padding = range * 0.1f + 1.0f;
            g_linear_accel_y_min = g_linear_accel_data_min - padding;
            g_linear_accel_y_max = g_linear_accel_data_max + padding;
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
        
        // Draw velocity plot
        if (show_velocity) {
            if (ImPlot::BeginPlot("Velocity", ImVec2(window_width, g_plot_height), ImPlotFlags_NoTitle)) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, "Linear Velocity", ImPlotAxisFlags_None);
                ImPlot::SetupAxisLimits(ImAxis_X1, g_x_min, g_x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_velocity_y_min, g_velocity_y_max, ImGuiCond_Always);
                
                std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
                if (!g_sensor_data.times.empty()) {
                    ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), 2.0f);
                    ImPlot::PlotLine("X", g_sensor_data.times.data(), g_sensor_data.vx_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Y", g_sensor_data.times.data(), g_sensor_data.vy_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.2f, 1.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Z", g_sensor_data.times.data(), g_sensor_data.vz_data.data(), g_sensor_data.times.size());
                }
                ImPlot::EndPlot();
            }
        }
        
        // Draw gravity plot
        if (show_gravity) {
            if (ImPlot::BeginPlot("Gravity", ImVec2(window_width, g_plot_height), ImPlotFlags_NoTitle)) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, "Gravity", ImPlotAxisFlags_None);
                ImPlot::SetupAxisLimits(ImAxis_X1, g_x_min, g_x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_gravity_y_min, g_gravity_y_max, ImGuiCond_Always);
                
                std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
                if (!g_sensor_data.times.empty()) {
                    ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("X", g_sensor_data.times.data(), g_sensor_data.gravity_x_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Y", g_sensor_data.times.data(), g_sensor_data.gravity_y_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Z", g_sensor_data.times.data(), g_sensor_data.gravity_z_data.data(), g_sensor_data.times.size());
                }
                ImPlot::EndPlot();
            }
        }
        
        // Draw linear acceleration plot
        if (show_linear_accel) {
            if (ImPlot::BeginPlot("Linear Acceleration", ImVec2(window_width, g_plot_height), ImPlotFlags_NoTitle)) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, "Linear Acceleration", ImPlotAxisFlags_None);
                ImPlot::SetupAxisLimits(ImAxis_X1, g_x_min, g_x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_linear_accel_y_min, g_linear_accel_y_max, ImGuiCond_Always);
                
                std::lock_guard<std::mutex> lock(g_sensor_data.mtx);
                if (!g_sensor_data.times.empty()) {
                    ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("X", g_sensor_data.times.data(), g_sensor_data.linear_ax_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Y", g_sensor_data.times.data(), g_sensor_data.linear_ay_data.data(), g_sensor_data.times.size());
                    
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), 2.0f);
                    ImPlot::PlotLine("Z", g_sensor_data.times.data(), g_sensor_data.linear_az_data.data(), g_sensor_data.times.size());
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
            ImGui::Checkbox("Velocity", &show_velocity);
            ImGui::Checkbox("Gravity", &show_gravity);
            ImGui::Checkbox("Linear Acceleration", &show_linear_accel);
            
            ImGui::Separator();
            
            if (ImGui::TreeNode("Accelerometer Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_accel_auto_fit);
                if (!g_accel_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_accel_y_min, -32768.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_accel_y_max, 0.0f, 32767.0f);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Gyroscope Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_gyro_auto_fit);
                if (!g_gyro_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_gyro_y_min, -32768.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_gyro_y_max, 0.0f, 32767.0f);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Velocity Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_velocity_auto_fit);
                if (!g_velocity_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_velocity_y_min, -1000.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_velocity_y_max, 0.0f, 1000.0f);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Gravity Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_gravity_auto_fit);
                if (!g_gravity_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_gravity_y_min, -10.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_gravity_y_max, 0.0f, 10.0f);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Linear Acceleration Y-Axis Settings")) {
                ImGui::Checkbox("Auto-fit Y-Axis", &g_linear_accel_auto_fit);
                if (!g_linear_accel_auto_fit) {
                    ImGui::SliderFloat("Y Min", &g_linear_accel_y_min, -32768.0f, 0.0f);
                    ImGui::SliderFloat("Y Max", &g_linear_accel_y_max, 0.0f, 32767.0f);
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