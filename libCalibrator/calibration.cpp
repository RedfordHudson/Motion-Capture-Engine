#include "calibration.h"
#include <iostream>
#include <chrono>
#include <numeric>
#include <algorithm>

namespace Calibration {

    // Helper function to get current time in seconds
    static double getCurrentTime() {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        return duration_cast<duration<double>>(now.time_since_epoch()).count();
    }

    // Helper function to calculate average of a vector of integers
    static double calculateAverage(const std::vector<int>& values) {
        if (values.empty()) return 0.0;
        
        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / values.size();
    }

    Calibrator::Calibrator()
        : m_isCalibrating(false)
        , m_calibrationDuration(0)
        , m_remainingTime(0)
        , m_callback(nullptr)
        , m_completeCallback(nullptr)
        , m_lastUpdateTime(0.0)
    {
        // Initialize vectors with a reasonable capacity
        m_ax_samples.reserve(500);
        m_ay_samples.reserve(500);
        m_az_samples.reserve(500);
        m_gx_samples.reserve(500);
        m_gy_samples.reserve(500);
        m_gz_samples.reserve(500);
    }

    void Calibrator::startCalibration(int duration, CalibrationCallback callback, CalibrationCompleteCallback completeCallback) {
        if (m_isCalibrating) {
            return; // Already calibrating
        }

        // Clear any previous calibration data
        m_ax_samples.clear();
        m_ay_samples.clear();
        m_az_samples.clear();
        m_gx_samples.clear();
        m_gy_samples.clear();
        m_gz_samples.clear();

        // Reset results
        m_results = CalibrationResults();

        m_isCalibrating = true;
        m_calibrationDuration = duration;
        m_remainingTime = duration;
        m_callback = callback;
        m_completeCallback = completeCallback;
        m_lastUpdateTime = getCurrentTime();

        std::cout << "Beginning calibration" << std::endl;
        if (m_callback) {
            m_callback("Beginning calibration");
        }
    }

    bool Calibrator::isCalibrating() const {
        return m_isCalibrating;
    }

    int Calibrator::getRemainingTime() const {
        return m_remainingTime;
    }

    void Calibrator::update(const std::unordered_map<std::string, int>* sensorData) {
        if (!m_isCalibrating) {
            return;
        }

        // Collect sensor data if provided
        if (sensorData != nullptr) {
            // Check that all expected keys exist
            if (sensorData->count("ax") && sensorData->count("ay") && sensorData->count("az") &&
                sensorData->count("gx") && sensorData->count("gy") && sensorData->count("gz")) {
                
                // Store samples
                m_ax_samples.push_back(sensorData->at("ax"));
                m_ay_samples.push_back(sensorData->at("ay"));
                m_az_samples.push_back(sensorData->at("az"));
                m_gx_samples.push_back(sensorData->at("gx"));
                m_gy_samples.push_back(sensorData->at("gy"));
                m_gz_samples.push_back(sensorData->at("gz"));
            }
        }

        // Calculate elapsed time since last update
        double currentTime = getCurrentTime();
        double elapsedSeconds = currentTime - m_lastUpdateTime;

        // Only update once per second
        if (elapsedSeconds >= 1.0) {
            m_lastUpdateTime = currentTime;
            m_remainingTime--;

            // Output the current count
            if (m_remainingTime > 0) {
                std::cout << m_remainingTime << std::endl;
                if (m_callback) {
                    m_callback(std::to_string(m_remainingTime));
                }
            }
            // Calibration complete
            else {
                processCollectedData();
                
                std::cout << "Calibration complete" << std::endl;
                std::cout << "Samples collected: " << m_results.sample_count << std::endl;
                std::cout << "Average values:" << std::endl;
                std::cout << "  ax: " << m_results.ax_avg << std::endl;
                std::cout << "  ay: " << m_results.ay_avg << std::endl;
                std::cout << "  az: " << m_results.az_avg << std::endl;
                std::cout << "  gx: " << m_results.gx_avg << std::endl;
                std::cout << "  gy: " << m_results.gy_avg << std::endl;
                std::cout << "  gz: " << m_results.gz_avg << std::endl;
                
                if (m_callback) {
                    m_callback("Calibration complete");
                }
                
                if (m_completeCallback) {
                    m_completeCallback(m_results);
                }
                
                m_isCalibrating = false;
            }
        }
    }

    void Calibrator::processCollectedData() {
        // Calculate sample count
        m_results.sample_count = m_ax_samples.size();
        
        // Calculate averages
        m_results.ax_avg = calculateAverage(m_ax_samples);
        m_results.ay_avg = calculateAverage(m_ay_samples);
        m_results.az_avg = calculateAverage(m_az_samples);
        m_results.gx_avg = calculateAverage(m_gx_samples);
        m_results.gy_avg = calculateAverage(m_gy_samples);
        m_results.gz_avg = calculateAverage(m_gz_samples);
    }

    const CalibrationResults& Calibrator::getResults() const {
        return m_results;
    }

} // namespace Calibration
