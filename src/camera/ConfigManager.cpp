#include "camera/ConfigManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>

using json = nlohmann::json;

ConfigManager::Params ConfigManager::params_;
static std::string last_court_file_;
static time_t last_court_mtime_ = 0;

bool ConfigManager::ReloadCourtPointsIfNeeded() {
    // 读取主 config
    std::ifstream f("../config/camera_config.json");
    if (!f.is_open()) return false;
    json data = json::parse(f);

    if (!data.contains("court_config") ||
        !data["court_config"].contains("default") ||
        !data["court_config"].contains("user")) {
        return false;
    }
    std::string default_court = data["court_config"]["default"];
    std::string user_court = data["court_config"]["user"];
    std::string court_to_use = default_court;

    struct stat st;
    time_t mtime = 0;
    if (access(user_court.c_str(), F_OK) == 0 && stat(user_court.c_str(), &st) == 0) {
        mtime = st.st_mtime;
        if ((std::time(nullptr) - mtime) < 86400) {
            court_to_use = user_court;
        }
    }

    // 如果court文件和mtime都没变，不需要reload
    if (court_to_use == last_court_file_ && mtime == last_court_mtime_) {
        return false;
    }

    // 重新加载court_points
    std::ifstream fc(court_to_use);
    if (!fc.is_open()) return false;
    json court_data = json::parse(fc);
    if (!court_data.contains("court_points")) return false;

    params_.court_points.clear();
    for (const auto& pt : court_data["court_points"]) {
        if (!pt.contains("x") || !pt.contains("y")) continue;
        params_.court_points.push_back({pt["x"], pt["y"]});
    }
    last_court_file_ = court_to_use;
    last_court_mtime_ = mtime;
    return true;
}

static bool IsFileRecent(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    std::time_t now = std::time(nullptr);
    // 86400 秒 = 1 天
    return (now - st.st_mtime) < 86400;
}

void ConfigManager::Initialize(const std::string& config_path) {
    // 1. 读取主 config（camera_config.json 或 user_camera_config.json）
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

        // 读取 transfer 配置
        if (!data.contains("transfer")) {
            throw std::runtime_error("Missing 'transfer' section in config");
        }
        const auto& t = data["transfer"];
        params_.transfer.x_min = t["x_min"];
        params_.transfer.x_max = t["x_max"];
        params_.transfer.y_min = t["y_min"];
        params_.transfer.y_max = t["y_max"];
        params_.transfer.fov_min = t["fov_min"];
        params_.transfer.fov_max = t["fov_max"];

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

        // 2. 读取 court_points（default/user court config 路径从主 config 读取）
        if (!data.contains("court_config") || 
            !data["court_config"].contains("default") ||
            !data["court_config"].contains("user")) {
            throw std::runtime_error("Missing 'court_config' or its paths in config");
        }
        std::string default_court = data["court_config"]["default"];
        std::string user_court = data["court_config"]["user"];
        std::string court_to_use = default_court;
        if (access(user_court.c_str(), F_OK) == 0 && IsFileRecent(user_court)) {
            court_to_use = user_court;
        }
        std::ifstream fc(court_to_use);
        if (!fc.is_open()) {
            throw std::runtime_error("Court config file not found: " + court_to_use);
        }
        json court_data = json::parse(fc);
        if (!court_data.contains("court_points")) {
            throw std::runtime_error("Missing 'court_points' in court config");
        }
        params_.court_points.clear();
        for (const auto& pt : court_data["court_points"]) {
            if (!pt.contains("x") || !pt.contains("y")) {
                throw std::runtime_error("Invalid court point format");
            }
            params_.court_points.push_back({pt["x"], pt["y"]});
        }

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