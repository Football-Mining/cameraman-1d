#pragma once
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <vector>

struct Point { float x, y; }; 

class ConfigManager {
public:
    static bool ReloadCourtPointsIfNeeded();
    struct Params {
        struct KalmanParams {
            float variance_position;
            float variance_measurement;
            float process_noise;
        };

        struct CameraParams {
            float memory_length;
            int fps;
            float speed_max;
            int buffer_pixels;
            float position_merge_ratio;
            float speed_slider_gain;
        };
        struct TransferParams {
            float x_min, x_max;
            float y_min, y_max;
            float fov_min, fov_max;
        };
        struct SafetyParams {
            int noise_threshold;
            int min_players;
            int boundary_margin;
        };

        KalmanParams base;
        KalmanParams slider;
        CameraParams camera;
        SafetyParams safety;
        TransferParams transfer;           // <--- 补充
        std::vector<Point> court_points;   // <--- 补充
    };

    static void Initialize(const std::string& config_path);
    static const Params& Get();

private:
    static Params params_;
    static void ValidateParams();
    static void ParseKalmanParams(const nlohmann::json& j, Params::KalmanParams& params);
};