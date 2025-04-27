#pragma once

#include <unordered_map>
#include <deque>

namespace Uncoupler {

    // Structure to hold uncoupled sensor data
    struct UncoupledData {
        // Raw accelerometer data (with gravity)
        float ax_raw = 0.0f;
        float ay_raw = 0.0f;
        float az_raw = 0.0f;
        
        // Calibrated gyroscope data
        float gx_cal = 0.0f;
        float gy_cal = 0.0f;
        float gz_cal = 0.0f;
        
        // Gravity components (estimated direction of gravity)
        float grav_x = 0.0f;
        float grav_y = 0.0f;
        float grav_z = 0.0f;
        
        // Linear acceleration (acceleration with gravity removed)
        float ax_linear = 0.0f;
        float ay_linear = 0.0f;
        float az_linear = 0.0f;
    };

    /**
     * Class to handle uncoupling of linear acceleration from rotational acceleration
     */
    class SensorUncoupler {
    public:
        /**
         * Constructor
         * @param gravity_filter_size Size of the moving average filter for gravity estimation
         */
        SensorUncoupler(size_t gravity_filter_size = 50);

        /**
         * Set gyroscope calibration offsets
         * @param gx_offset X-axis gyroscope offset
         * @param gy_offset Y-axis gyroscope offset
         * @param gz_offset Z-axis gyroscope offset
         */
        void setGyroCalibrationOffsets(float gx_offset, float gy_offset, float gz_offset);

        /**
         * Process sensor data to separate linear and rotational components
         * @param sensorData Raw sensor data from IMU
         * @return Uncoupled sensor data with calibrated gyro values
         */
        UncoupledData processData(const std::unordered_map<std::string, int>& sensorData);

        /**
         * Get the estimated gravity vector
         * @return Array of 3 floats [x, y, z] representing gravity direction
         */
        const float* getGravityVector() const;

        /**
         * Enable/disable gyroscope calibration
         * @param enable True to enable calibration, false to disable
         */
        void enableGyroCalibration(bool enable);

        /**
         * Check if gyroscope calibration is enabled
         * @return True if calibration is enabled
         */
        bool isGyroCalibrationEnabled() const;

    private:
        // Apply calibration to gyroscope value
        int applyGyroCalibration(int value, float offset) const;
        
        // Update gravity vector estimation with new accelerometer data
        void updateGravityEstimation(float ax, float ay, float az);
        
        // Normalize a 3D vector
        void normalizeVector(float& x, float& y, float& z);

        // Gyroscope calibration offsets
        float m_gx_offset;
        float m_gy_offset;
        float m_gz_offset;
        
        // Calibration state
        bool m_gyroCalibrationEnabled;
        
        // Gravity vector estimation
        float m_gravity_vector[3];  // Estimated gravity direction (normalized)
        float m_gravity_magnitude;  // Estimated gravity magnitude
        
        // Moving average filters for gravity estimation
        std::deque<float> m_ax_history;
        std::deque<float> m_ay_history;
        std::deque<float> m_az_history;
        size_t m_gravity_filter_size;
    };

} // namespace Uncoupler
