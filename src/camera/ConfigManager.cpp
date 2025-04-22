#include "camera/ConfigManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ConfigManager::Params ConfigManager::params_;

void ConfigManager::Initialize(const std::string& config_path) {
    std::ifstream f(config_path);
    if (!f.is_open()) {
        throw std::runtime_error("Config file not found: " + config_path);
    }

    try {
        json data = json::parse(f);

        // 检查 kalman 配置
        if (!data.contains("kalman")) {
            throw std::runtime_error("Missing 'kalman' section in config");
        }
        json& kalman = data["kalman"];

        if (!kalman.contains("base")) {
            throw std::runtime_error("Missing 'base' in kalman config");
        }
        ParseKalmanParams(kalman["base"], params_.base);

        if (!kalman.contains("slider")) {
            throw std::runtime_error("Missing 'slider' in kalman config");
        }
        ParseKalmanParams(kalman["slider"], params_.slider);

        // 检查 camera 配置
        if (!data.contains("camera")) {
            throw std::runtime_error("Missing 'camera' section in config");
        }
        json& cam = data["camera"];
        const std::vector<std::string> camera_keys = {
            "memory_length", "fps", "speed_max", 
            "buffer_pixels", "position_merge_ratio", "speed_slider_gain"
        };
        for (const auto& key : camera_keys) {
            if (!cam.contains(key)) {
                throw std::runtime_error("Missing key in camera config: " + key);
            }
        }
        params_.camera = {
            cam["memory_length"],
            cam["fps"],
            cam["speed_max"],
            cam["buffer_pixels"],
            cam["position_merge_ratio"],
            cam["speed_slider_gain"]
        };

        // 检查 safety 配置
        if (!data.contains("safety")) {
            throw std::runtime_error("Missing 'safety' section in config");
        }
        json& safety = data["safety"];
        const std::vector<std::string> safety_keys = {
            "noise_threshold", "min_players", "boundary_margin"
        };
        for (const auto& key : safety_keys) {
            if (!safety.contains(key)) {
                throw std::runtime_error("Missing key in safety config: " + key);
            }
        }
        params_.safety = {
            safety["noise_threshold"],
            safety["min_players"],
            safety["boundary_margin"]
        };

        ValidateParams();
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }
}

void ConfigManager::ParseKalmanParams(const json& j, Params::KalmanParams& params) {
    const std::vector<std::string> required_keys = {
        "variance_position", "variance_measurement", "process_noise"
    };
    for (const auto& key : required_keys) {
        if (!j.contains(key)) {
            throw std::runtime_error("Missing key in Kalman config: " + key);
        }
    }
    params.variance_position = j["variance_position"];
    params.variance_measurement = j["variance_measurement"];
    params.process_noise = j["process_noise"];
}
void ConfigManager::ValidateParams() {
    if (params_.camera.memory_length <= 0) throw std::invalid_argument("Invalid memory_length");
    if (params_.camera.fps <= 0) throw std::invalid_argument("Invalid fps");
    // 其他参数验证...
}

const ConfigManager::Params& ConfigManager::Get() {
    return params_;
}