#pragma once
#include "camera/KalmanFilter1D.hpp"  // 添加这行
#include "camera/ConfigManager.hpp"    // 添加这行
#include <deque>
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map> 

class CameramanModel {
public:
    explicit CameramanModel(const std::vector<Point>& court_points);
    
    float predict(
        const std::vector<Point>& player_positions,
        const std::vector<Point>& ball_positions
    );

    struct DebugInfo {
        float raw_target;
        float filtered_target;
        float mean_player_pos;
        float calculated_speed;
        float focus_slider;
    };

    std::optional<DebugInfo> getDebugInfo() const;
    std::tuple<float, float> transfer(float x) const;

private:
    void initializeHistory(const std::vector<Point>& positions);
    float calculateAccumulatedSpeed(const std::deque<float>& positions) const;
    void handleEmptyInput(std::vector<Point>& players, std::vector<Point>& balls) const;

    std::deque<float> player_pos_memory_ = {0.f};
    std::deque<float> player_max_memory_;
    std::deque<float> player_min_memory_;
    std::deque<float> ball_pos_memory_;

    std::unique_ptr<KalmanFilter1D> slider_filter_;
    std::unordered_map<std::string, std::unique_ptr<KalmanFilter1D>> kalman_filters_;

    const ConfigManager::Params& params_;
    float left_most_;
    float right_most_;
    bool initialized_ = false;
    mutable std::optional<DebugInfo> debug_info_;
};