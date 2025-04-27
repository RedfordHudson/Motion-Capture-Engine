#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>

namespace Calibration {

    // Structure to hold calibration results
    struct CalibrationResults {
        double ax_avg = 0.0;
        double ay_avg = 0.0;
        double az_avg = 0.0;
        double gx_avg = 0.0;
        double gy_avg = 0.0;
        double gz_avg = 0.0;
        int sample_count = 0;
    };

    // Callback type for calibration events
    using CalibrationCallback = std::function<void(const std::string&)>;
    
    // Callback type for when calibration completes
    using CalibrationCompleteCallback = std::function<void(const CalibrationResults&)>;

    /**
     * Class to handle basic sensor calibration
     */
    class Calibrator {
    public:
        /**
         * Constructor
         */
        Calibrator();

        /**
         * Start the calibration process
         * @param duration Duration in seconds for the countdown
         * @param callback Optional callback to receive calibration events
         * @param completeCallback Optional callback for when calibration is complete with results
         */
        void startCalibration(int duration = 5, 
                             CalibrationCallback callback = nullptr,
                             CalibrationCompleteCallback completeCallback = nullptr);

        /**
         * Check if calibration is in progress
         * @return True if calibration is active
         */
        bool isCalibrating() const;

        /**
         * Get the remaining time in the calibration
         * @return Seconds remaining, or 0 if not calibrating
         */
        int getRemainingTime() const;

        /**
         * Process a calibration step (call this regularly to update the calibration state)
         * @param sensorData Current sensor readings to collect during calibration
         */
        void update(const std::unordered_map<std::string, int>* sensorData = nullptr);

        /**
         * Get the latest calibration results
         * @return The calculated calibration results
         */
        const CalibrationResults& getResults() const;

    private:
        // Process the collected data and calculate averages
        void processCollectedData();

        bool m_isCalibrating;
        int m_calibrationDuration;
        int m_remainingTime;
        CalibrationCallback m_callback;
        CalibrationCompleteCallback m_completeCallback;
        double m_lastUpdateTime;

        // Accumulator for sensor data
        std::vector<int> m_ax_samples;
        std::vector<int> m_ay_samples;
        std::vector<int> m_az_samples;
        std::vector<int> m_gx_samples;
        std::vector<int> m_gy_samples;
        std::vector<int> m_gz_samples;

        // Calibration results
        CalibrationResults m_results;
    };

} // namespace Calibration
