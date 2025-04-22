#pragma once
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>

class ConfigManager {
public:
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

        struct SafetyParams {
            int noise_threshold;
            int min_players;
            int boundary_margin;
        };

        KalmanParams base;
        KalmanParams slider;
        CameraParams camera;
        SafetyParams safety;
    };

    static void Initialize(const std::string& config_path);
    static const Params& Get();

private:
    static Params params_;
    static void ValidateParams();
    static void ParseKalmanParams(const nlohmann::json& j, Params::KalmanParams& params);
};