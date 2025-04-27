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

    // Plot visibility flags - renamed for clarity
    static bool accel_acc = true;  // Accelerometer acceleration
    static bool gyro_acc = true;   // Gyroscope acceleration
    static bool accel_vel = true;  // Accelerometer velocity
    static bool gyro_vel = true;   // Gyroscope velocity
    static bool accel_pos = true;  // Accelerometer position
    static bool gyro_pos = true;   // Gyroscope position
    
    // Plot settings
    static float g_plot_height = 250.0f;  // Default height for each plot
    static float g_time_window = 5.0f;    // Show 5 seconds of data
    static bool g_auto_fit_y = true;      // Auto-fit only the Y axis
    static float g_y_min = -2000.0f;
    static float g_y_max = 2000.0f;
    
    // Debug flags
    static bool g_debug_mode = true;      // Enable debug output
    
    // Auto detection of data range
    static float g_data_min = -2000.0f;
    static float g_data_max = 2000.0f;
    static bool g_auto_detect_range = true;
    
    // Colors for each dimension
    static const ImVec4 g_colors[] = {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red - X
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green - Y
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue - Z
    };

    bool initialize(const std::string& title) {
        // Set error callback
        glfwSetErrorCallback([](int error, const char* description) {
            std::cerr << "GLFW Error " << error << ": " << description << std::endl;
        });
        
        // Initialize GLFW
        if (!glfwInit())
            return false;

        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        // Create window with graphics context - taller window to accommodate vertical plots
        g_window = glfwCreateWindow(1600, 1200, title.c_str(), NULL, NULL);
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
        
        // Accelerometer data
        g_sensorData.ax_data.reserve(MAX_POINTS);
        g_sensorData.ay_data.reserve(MAX_POINTS);
        g_sensorData.az_data.reserve(MAX_POINTS);
        
        // Gyroscope data
        g_sensorData.gx_data.reserve(MAX_POINTS);
        g_sensorData.gy_data.reserve(MAX_POINTS);
        g_sensorData.gz_data.reserve(MAX_POINTS);
        
        // Accelerometer-derived velocity
        g_sensorData.avx_data.reserve(MAX_POINTS);
        g_sensorData.avy_data.reserve(MAX_POINTS);
        g_sensorData.avz_data.reserve(MAX_POINTS);
        
        // Gyroscope-derived velocity
        g_sensorData.gvx_data.reserve(MAX_POINTS);
        g_sensorData.gvy_data.reserve(MAX_POINTS);
        g_sensorData.gvz_data.reserve(MAX_POINTS);
        
        // Accelerometer-derived position
        g_sensorData.apx_data.reserve(MAX_POINTS);
        g_sensorData.apy_data.reserve(MAX_POINTS);
        g_sensorData.apz_data.reserve(MAX_POINTS);
        
        // Gyroscope-derived position
        g_sensorData.gpx_data.reserve(MAX_POINTS);
        g_sensorData.gpy_data.reserve(MAX_POINTS);
        g_sensorData.gpz_data.reserve(MAX_POINTS);
        
        std::cout << "Plot system initialized with window size: 1600x1200" << std::endl;
        
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

    void configurePlots(bool show_accel, bool show_gyro, bool show_accel_vel, 
                       bool show_gyro_vel, bool show_accel_pos, bool show_gyro_pos) {
        // Update visibility flags with clear variable names
        accel_acc = show_accel;
        gyro_acc = show_gyro;
        accel_vel = show_accel_vel;
        gyro_vel = show_gyro_vel;
        accel_pos = show_accel_pos;
        gyro_pos = show_gyro_pos;
        
        // Always print configuration for debugging
        std::cout << "Plot configuration: "
                  << "accel_acc=" << (accel_acc ? "ON" : "off") << ", "
                  << "gyro_acc=" << (gyro_acc ? "ON" : "off") << ", "
                  << "accel_vel=" << (accel_vel ? "ON" : "off") << ", "
                  << "gyro_vel=" << (gyro_vel ? "ON" : "off") << ", "
                  << "accel_pos=" << (accel_pos ? "ON" : "off") << ", "
                  << "gyro_pos=" << (gyro_pos ? "ON" : "off") << std::endl;
    }

    void addDataPoint(
        const std::unordered_map<std::string, int>& sensor_data,
        const Hand::Vector3D& accel_velocity,
        const Hand::Vector3D& accel_position,
        const Hand::Vector3D& gyro_velocity,
        const Hand::Vector3D& gyro_position
    ) {
        if (!g_initialized)
            return;
            
        std::lock_guard<std::mutex> lock(g_sensorData.mtx);
        
        // Update time
        g_time += 0.01f; // Assume 100Hz sampling rate
        
        // Add time point
        g_sensorData.times.push_back(g_time);
        
        // Add accelerometer data
        float ax = static_cast<float>(sensor_data.at("ax"));
        float ay = static_cast<float>(sensor_data.at("ay"));
        float az = static_cast<float>(sensor_data.at("az"));
        g_sensorData.ax_data.push_back(ax);
        g_sensorData.ay_data.push_back(ay);
        g_sensorData.az_data.push_back(az);
        
        // Add gyroscope data
        float gx = static_cast<float>(sensor_data.at("gx"));
        float gy = static_cast<float>(sensor_data.at("gy"));
        float gz = static_cast<float>(sensor_data.at("gz"));
        g_sensorData.gx_data.push_back(gx);
        g_sensorData.gy_data.push_back(gy);
        g_sensorData.gz_data.push_back(gz);
        
        // Add accelerometer-derived velocity
        g_sensorData.avx_data.push_back(accel_velocity.x);
        g_sensorData.avy_data.push_back(accel_velocity.y);
        g_sensorData.avz_data.push_back(accel_velocity.z);
        
        // Add gyroscope-derived velocity
        g_sensorData.gvx_data.push_back(gyro_velocity.x);
        g_sensorData.gvy_data.push_back(gyro_velocity.y);
        g_sensorData.gvz_data.push_back(gyro_velocity.z);
        
        // Add accelerometer-derived position
        g_sensorData.apx_data.push_back(accel_position.x);
        g_sensorData.apy_data.push_back(accel_position.y);
        g_sensorData.apz_data.push_back(accel_position.z);
        
        // Add gyroscope-derived position
        g_sensorData.gpx_data.push_back(gyro_position.x);
        g_sensorData.gpy_data.push_back(gyro_position.y);
        g_sensorData.gpz_data.push_back(gyro_position.z);
        
        // Debug output for first few data points
        static int debug_counter = 0;
        if (g_debug_mode && debug_counter < 5) {
            std::cout << "Data point added at t=" << g_time 
                      << " ax=" << ax << " ay=" << ay << " az=" << az
                      << " gx=" << gx << " gy=" << gy << " gz=" << gz << std::endl;
            debug_counter++;
        }
        
        // Auto-detect data range for Y-axis limits
        if (g_auto_detect_range && !g_sensorData.times.empty()) {
            float min_val = std::numeric_limits<float>::max();
            float max_val = std::numeric_limits<float>::lowest();
            
            // Update range for accelerometer data
            if (accel_acc && !g_sensorData.ax_data.empty()) {
                min_val = std::min(min_val, *std::min_element(g_sensorData.ax_data.begin(), g_sensorData.ax_data.end()));
                min_val = std::min(min_val, *std::min_element(g_sensorData.ay_data.begin(), g_sensorData.ay_data.end()));
                min_val = std::min(min_val, *std::min_element(g_sensorData.az_data.begin(), g_sensorData.az_data.end()));
                
                max_val = std::max(max_val, *std::max_element(g_sensorData.ax_data.begin(), g_sensorData.ax_data.end()));
                max_val = std::max(max_val, *std::max_element(g_sensorData.ay_data.begin(), g_sensorData.ay_data.end()));
                max_val = std::max(max_val, *std::max_element(g_sensorData.az_data.begin(), g_sensorData.az_data.end()));
            }
            
            // Update range for gyroscope data
            if (gyro_acc && !g_sensorData.gx_data.empty()) {
                min_val = std::min(min_val, *std::min_element(g_sensorData.gx_data.begin(), g_sensorData.gx_data.end()));
                min_val = std::min(min_val, *std::min_element(g_sensorData.gy_data.begin(), g_sensorData.gy_data.end()));
                min_val = std::min(min_val, *std::min_element(g_sensorData.gz_data.begin(), g_sensorData.gz_data.end()));
                
                max_val = std::max(max_val, *std::max_element(g_sensorData.gx_data.begin(), g_sensorData.gx_data.end()));
                max_val = std::max(max_val, *std::max_element(g_sensorData.gy_data.begin(), g_sensorData.gy_data.end()));
                max_val = std::max(max_val, *std::max_element(g_sensorData.gz_data.begin(), g_sensorData.gz_data.end()));
            }
            
            // Only update if we have valid min/max values
            if (min_val != std::numeric_limits<float>::max() && max_val != std::numeric_limits<float>::lowest()) {
                g_data_min = min_val;
                g_data_max = max_val;
                
                // Add padding
                float range = g_data_max - g_data_min;
                float padding = range * 0.1f;
                
                // Update y-axis limits if using auto-fit
                if (g_auto_fit_y) {
                    g_y_min = g_data_min - padding;
                    g_y_max = g_data_max + padding;
                }
            }
        }
        
        // Limit history size
        if (g_sensorData.times.size() > MAX_POINTS) {
            g_sensorData.times.erase(g_sensorData.times.begin());
            g_sensorData.ax_data.erase(g_sensorData.ax_data.begin());
            g_sensorData.ay_data.erase(g_sensorData.ay_data.begin());
            g_sensorData.az_data.erase(g_sensorData.az_data.begin());
            g_sensorData.gx_data.erase(g_sensorData.gx_data.begin());
            g_sensorData.gy_data.erase(g_sensorData.gy_data.begin());
            g_sensorData.gz_data.erase(g_sensorData.gz_data.begin());
            g_sensorData.avx_data.erase(g_sensorData.avx_data.begin());
            g_sensorData.avy_data.erase(g_sensorData.avy_data.begin());
            g_sensorData.avz_data.erase(g_sensorData.avz_data.begin());
            g_sensorData.gvx_data.erase(g_sensorData.gvx_data.begin());
            g_sensorData.gvy_data.erase(g_sensorData.gvy_data.begin());
            g_sensorData.gvz_data.erase(g_sensorData.gvz_data.begin());
            g_sensorData.apx_data.erase(g_sensorData.apx_data.begin());
            g_sensorData.apy_data.erase(g_sensorData.apy_data.begin());
            g_sensorData.apz_data.erase(g_sensorData.apz_data.begin());
            g_sensorData.gpx_data.erase(g_sensorData.gpx_data.begin());
            g_sensorData.gpy_data.erase(g_sensorData.gpy_data.begin());
            g_sensorData.gpz_data.erase(g_sensorData.gpz_data.begin());
        }
    }

    // Draw plots in a much simpler way to ensure they're visible
    static void drawPlots() {
        if (!g_initialized)
            return;
            
        if (g_sensorData.times.empty()) {
            if (g_debug_mode) {
                std::cout << "No data to plot yet." << std::endl;
            }
            return;
        }
            
        std::lock_guard<std::mutex> lock(g_sensorData.mtx);
        
        // Create a full-window ImGui window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        if (!ImGui::Begin("Sensor Data", nullptr, 
                  ImGuiWindowFlags_NoTitleBar | 
                  ImGuiWindowFlags_NoResize | 
                  ImGuiWindowFlags_NoMove | 
                  ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            return;
        }
        
        // Debug information
        if (g_debug_mode) {
            ImGui::Text("Debug Info: Window Size: %.0fx%.0f, Data points: %d", 
                       ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 
                       (int)g_sensorData.times.size());
            
            ImGui::Text("Plot Visibility: Accel=%d, Gyro=%d, AcVel=%d, GyVel=%d, AcPos=%d, GyPos=%d",
                       (int)accel_acc, (int)gyro_acc, (int)accel_vel, (int)gyro_vel, (int)accel_pos, (int)gyro_pos);
            
            ImGui::Separator();
        }
        
        // Controls section
        if (ImGui::CollapsingHeader("Plot Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Debug Mode", &g_debug_mode);
            ImGui::SliderFloat("Plot Height", &g_plot_height, 100.0f, 400.0f);
            ImGui::SliderFloat("Time Window", &g_time_window, 1.0f, 20.0f);
            ImGui::Checkbox("Auto-fit Y-axis", &g_auto_fit_y);
            
            if (!g_auto_fit_y) {
                ImGui::SliderFloat("Y Min", &g_y_min, -10000.0f, 0.0f);
                ImGui::SliderFloat("Y Max", &g_y_max, 0.0f, 10000.0f);
            } else {
                ImGui::Text("Data Range: %.0f to %.0f", g_data_min, g_data_max);
                ImGui::Text("Y-axis Range: %.0f to %.0f", g_y_min, g_y_max);
            }
        }
        
        // Plot visibility toggles
        if (ImGui::CollapsingHeader("Plot Visibility", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Use more obvious UI elements for checkboxes (add colors and spacing)
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 100, 100, 255));
            bool accel_changed = ImGui::Checkbox("Accelerometer", &accel_acc);
            ImGui::PopStyleColor();
            
            ImGui::SameLine(200);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(100, 100, 255, 255));
            bool gyro_changed = ImGui::Checkbox("Gyroscope", &gyro_acc);
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 100, 100, 255));
            bool accel_vel_changed = ImGui::Checkbox("Accel. Velocity", &accel_vel);
            ImGui::PopStyleColor();
            
            ImGui::SameLine(200);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(100, 100, 255, 255));
            bool gyro_vel_changed = ImGui::Checkbox("Gyro. Velocity", &gyro_vel);
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 100, 100, 255));
            bool accel_pos_changed = ImGui::Checkbox("Accel. Position", &accel_pos);
            ImGui::PopStyleColor();
            
            ImGui::SameLine(200);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(100, 100, 255, 255));
            bool gyro_pos_changed = ImGui::Checkbox("Gyro. Position", &gyro_pos);
            ImGui::PopStyleColor();
            
            // Log checkbox changes to help debug
            if (g_debug_mode && (accel_changed || gyro_changed || accel_vel_changed || 
                                 gyro_vel_changed || accel_pos_changed || gyro_pos_changed)) {
                std::cout << "Visibility flags changed: "
                          << "accel_acc=" << accel_acc << ", "
                          << "gyro_acc=" << gyro_acc << ", "
                          << "accel_vel=" << accel_vel << ", "
                          << "gyro_vel=" << gyro_vel << ", "
                          << "accel_pos=" << accel_pos << ", "
                          << "gyro_pos=" << gyro_pos << std::endl;
            }
            
            // Button to enable all plots
            ImGui::Separator();
            if (ImGui::Button("Enable All Plots", ImVec2(150, 30))) {
                accel_acc = true;
                gyro_acc = true;
                accel_vel = true;
                gyro_vel = true;
                accel_pos = true;
                gyro_pos = true;
                
                if (g_debug_mode) {
                    std::cout << "All plots enabled" << std::endl;
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Disable All Plots", ImVec2(150, 30))) {
                accel_acc = false;
                gyro_acc = false;
                accel_vel = false;
                gyro_vel = false;
                accel_pos = false;
                gyro_pos = false;
                
                if (g_debug_mode) {
                    std::cout << "All plots disabled" << std::endl;
                }
            }
        }
        
        ImGui::Separator();
        
        // Set time window for x-axis
        float x_min = g_time - g_time_window;
        float x_max = g_time;
        
        // Count visible plots
        int visible_plots = 0;
        if (accel_acc) visible_plots++;
        if (gyro_acc) visible_plots++;
        if (accel_vel) visible_plots++;
        if (gyro_vel) visible_plots++;
        if (accel_pos) visible_plots++;
        if (gyro_pos) visible_plots++;
        
        if (visible_plots == 0) {
            ImGui::TextColored(ImVec4(1,0,0,1), "No plots are currently visible. Please enable at least one plot type.");
            ImGui::End();
            return;
        }
        
        // Create a scrollable region with explicit size calculation
        float available_height = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("PlotScroll", ImVec2(0, available_height), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        // Force use full width
        float content_width = ImGui::GetContentRegionAvail().x;
        ImVec2 plot_size(content_width, g_plot_height);
        
        // Track the plotting results for debugging
        bool plot_success = false;
        int plots_rendered = 0;
        
        // Accelerometer plot - using accel_acc
        if (accel_acc) {
            ImGui::TextColored(g_colors[0], "Accelerometer Plot");
            bool plot_started = ImPlot::BeginPlot("Accelerometer", plot_size);
            
            if (plot_started) {
                plot_success = true;
                plots_rendered++;
                
                ImPlot::SetupAxes("Time (s)", "Acceleration", 0, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_y_min, g_y_max, g_auto_fit_y ? ImGuiCond_Once : ImGuiCond_Always);
                
                if (!g_sensorData.times.empty()) {
                    ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                    ImPlot::PlotLine("X", g_sensorData.times.data(), g_sensorData.ax_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                    ImPlot::PlotLine("Y", g_sensorData.times.data(), g_sensorData.ay_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                    ImPlot::PlotLine("Z", g_sensorData.times.data(), g_sensorData.az_data.data(), g_sensorData.times.size());
                }
                ImPlot::EndPlot();
            }
            else if (g_debug_mode) {
                std::cout << "Failed to start Accelerometer plot" << std::endl;
            }
            
            ImGui::Separator();
        }
        
        // Gyroscope plot - using gyro_acc 
        if (gyro_acc) {
            ImGui::TextColored(g_colors[1], "Gyroscope Plot");
            bool plot_started = ImPlot::BeginPlot("Gyroscope", plot_size);
            
            if (plot_started) {
                plot_success = true;
                plots_rendered++;
                
                ImPlot::SetupAxes("Time (s)", "Gyroscope", 0, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_y_min, g_y_max, g_auto_fit_y ? ImGuiCond_Once : ImGuiCond_Always);
                
                if (!g_sensorData.times.empty()) {
                    ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                    ImPlot::PlotLine("X", g_sensorData.times.data(), g_sensorData.gx_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                    ImPlot::PlotLine("Y", g_sensorData.times.data(), g_sensorData.gy_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                    ImPlot::PlotLine("Z", g_sensorData.times.data(), g_sensorData.gz_data.data(), g_sensorData.times.size());
                }
                ImPlot::EndPlot();
            }
            else if (g_debug_mode) {
                std::cout << "Failed to start Gyroscope plot" << std::endl;
            }
            
            ImGui::Separator();
        }
        
        // Accelerometer velocity plot - using accel_vel
        if (accel_vel) {
            ImGui::TextColored(g_colors[0], "Accelerometer Velocity Plot");
            bool plot_started = ImPlot::BeginPlot("Accel. Velocity", plot_size);
            
            if (plot_started) {
                plot_success = true;
                plots_rendered++;
                
                ImPlot::SetupAxes("Time (s)", "Accel. Velocity", 0, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_y_min, g_y_max, g_auto_fit_y ? ImGuiCond_Once : ImGuiCond_Always);
                
                if (!g_sensorData.times.empty()) {
                    ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                    ImPlot::PlotLine("X", g_sensorData.times.data(), g_sensorData.avx_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                    ImPlot::PlotLine("Y", g_sensorData.times.data(), g_sensorData.avy_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                    ImPlot::PlotLine("Z", g_sensorData.times.data(), g_sensorData.avz_data.data(), g_sensorData.times.size());
                }
                ImPlot::EndPlot();
            }
            else if (g_debug_mode) {
                std::cout << "Failed to start Accel. Velocity plot" << std::endl;
            }
            
            ImGui::Separator();
        }
        
        // Gyroscope velocity plot - using gyro_vel
        if (gyro_vel) {
            ImGui::TextColored(g_colors[1], "Gyroscope Velocity Plot");
            bool plot_started = ImPlot::BeginPlot("Gyro. Velocity", plot_size);
            
            if (plot_started) {
                plot_success = true;
                plots_rendered++;
                
                ImPlot::SetupAxes("Time (s)", "Gyro. Velocity", 0, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_y_min, g_y_max, g_auto_fit_y ? ImGuiCond_Once : ImGuiCond_Always);
                
                if (!g_sensorData.times.empty()) {
                    ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                    ImPlot::PlotLine("X", g_sensorData.times.data(), g_sensorData.gvx_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                    ImPlot::PlotLine("Y", g_sensorData.times.data(), g_sensorData.gvy_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                    ImPlot::PlotLine("Z", g_sensorData.times.data(), g_sensorData.gvz_data.data(), g_sensorData.times.size());
                }
                ImPlot::EndPlot();
            }
            else if (g_debug_mode) {
                std::cout << "Failed to start Gyro. Velocity plot" << std::endl;
            }
            
            ImGui::Separator();
        }
        
        // Accelerometer position plot - using accel_pos
        if (accel_pos) {
            ImGui::TextColored(g_colors[0], "Accelerometer Position Plot");
            bool plot_started = ImPlot::BeginPlot("Accel. Position", plot_size);
            
            if (plot_started) {
                plot_success = true;
                plots_rendered++;
                
                ImPlot::SetupAxes("Time (s)", "Accel. Position", 0, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_y_min, g_y_max, g_auto_fit_y ? ImGuiCond_Once : ImGuiCond_Always);
                
                if (!g_sensorData.times.empty()) {
                    ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                    ImPlot::PlotLine("X", g_sensorData.times.data(), g_sensorData.apx_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                    ImPlot::PlotLine("Y", g_sensorData.times.data(), g_sensorData.apy_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                    ImPlot::PlotLine("Z", g_sensorData.times.data(), g_sensorData.apz_data.data(), g_sensorData.times.size());
                }
                ImPlot::EndPlot();
            }
            else if (g_debug_mode) {
                std::cout << "Failed to start Accel. Position plot" << std::endl;
            }
            
            ImGui::Separator();
        }
        
        // Gyroscope position plot - using gyro_pos
        if (gyro_pos) {
            ImGui::TextColored(g_colors[1], "Gyroscope Position Plot");
            bool plot_started = ImPlot::BeginPlot("Gyro. Position", plot_size);
            
            if (plot_started) {
                plot_success = true;
                plots_rendered++;
                
                ImPlot::SetupAxes("Time (s)", "Gyro. Position", 0, 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, g_y_min, g_y_max, g_auto_fit_y ? ImGuiCond_Once : ImGuiCond_Always);
                
                if (!g_sensorData.times.empty()) {
                    ImPlot::SetNextLineStyle(g_colors[0], 2.0f);
                    ImPlot::PlotLine("X", g_sensorData.times.data(), g_sensorData.gpx_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[1], 2.0f);
                    ImPlot::PlotLine("Y", g_sensorData.times.data(), g_sensorData.gpy_data.data(), g_sensorData.times.size());
                    
                    ImPlot::SetNextLineStyle(g_colors[2], 2.0f);
                    ImPlot::PlotLine("Z", g_sensorData.times.data(), g_sensorData.gpz_data.data(), g_sensorData.times.size());
                }
                ImPlot::EndPlot();
            }
            else if (g_debug_mode) {
                std::cout << "Failed to start Gyro. Position plot" << std::endl;
            }
            
            ImGui::Separator();
        }
        
        ImGui::EndChild();
        
        // Status bar at bottom
        ImGui::Separator();
        ImGui::Text("Data points: %d | Time: %.2f | Visible window: %.1f s", 
                  (int)g_sensorData.times.size(), g_time, g_time_window);
                  
        if (g_debug_mode) {
            ImGui::SameLine();
            ImGui::Text("| Visible Plots: %d/%d rendered", plots_rendered, visible_plots);
            
            if (!plot_success && visible_plots > 0) {
                ImGui::TextColored(ImVec4(1,0,0,1), "Warning: Failed to render some plots. Try reducing the number of visible plots.");
            }
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