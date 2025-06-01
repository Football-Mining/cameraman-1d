#include "camera/CameramanModel.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iostream>

void CameramanModel::initializeHistory(const std::vector<Point>& positions) {
    std::cout << "初始化历史数据，输入位置数量: " << positions.size() << std::endl;
    
    if (positions.empty()) {
        throw std::runtime_error("无法用空位置初始化历史数据");
    }

    const size_t history_size = 
        static_cast<size_t>(params_.camera.memory_length * params_.camera.fps);
    
    std::cout << "计算历史队列大小: " << history_size 
              << " (memory_length=" << params_.camera.memory_length
              << ", fps=" << params_.camera.fps << ")\n";

    if (history_size == 0) {
        throw std::invalid_argument("历史队列大小不能为0");
    }

    const float initial = positions[0].x;
    std::cout << "初始基准位置X: " << initial << std::endl;


    
    // 清空现有数据
    player_pos_memory_.clear();
    player_pos_memory_.resize(history_size, initial);
    std::cout << "位置队列初始化完成，实际大小: " 
              << player_pos_memory_.size() << std::endl;
    player_max_memory_.clear();
    player_min_memory_.clear();
    
    // 填充历史数据
    player_max_memory_.resize(history_size, initial);
    player_min_memory_.resize(history_size, initial);
}

std::optional<CameramanModel::DebugInfo> CameramanModel::getDebugInfo() const {
    return debug_info_;
}

void CameramanModel::handleEmptyInput(std::vector<Point>& players, 
                                    std::vector<Point>& balls) const {
    if (players.empty()) {
        players.push_back({player_pos_memory_.back(), 0});
    }
    if (balls.empty()) {
        balls.push_back({player_pos_memory_.back(), 0});
    }
}

float CameramanModel::calculateAccumulatedSpeed(const std::deque<float>& positions) const {
    if (positions.size() < 2) return 0.0f;
    
    float total = 0.0f;
    for (size_t i = 1; i < positions.size(); ++i) {
        total += positions[i] - positions[i-1];
    }
    return total * params_.camera.fps; // 转换为每秒速度
}
// CameramanModel.cpp
CameramanModel::CameramanModel(const std::vector<Point>& court_points)
    : params_(ConfigManager::Get()),
      left_most_(0.0f),    // 直接在初始化列表赋值
      right_most_(1920.0f), 
      initialized_(false) 
{
    if (!court_points.empty()) {
        auto x_comparator = [](const Point& a, const Point& b) { 
            return a.x < b.x; 
        };
        
        auto min_it = std::min_element(court_points.begin(), 
                                      court_points.end(), 
                                      x_comparator);
        auto max_it = std::max_element(court_points.begin(),
                                      court_points.end(),
                                      x_comparator);
        
        left_most_ = min_it->x;    // 允许修改非 const 成员
        right_most_ = max_it->x;   // 允许修改非 const 成员
    

        std::cout << "计算出的球场边界: left=" << left_most_ 
                  << ", right=" << right_most_ << std::endl;
    } else {
        std::cout << "警告: 使用默认球场边界 (0, 1920)\n";
    }

    // 检查 Kalman 滤波器参数
    std::cout << "Slider滤波参数 - variance_position: " 
              << ConfigManager::Get().slider.variance_position
              << ", variance_measurement: " 
              << ConfigManager::Get().slider.variance_measurement 
              << std::endl;

    // 初始化滤波器
    try {
        const auto& slider_params = ConfigManager::Get().slider;

        // 正确调用构造函数（2个参数）
        slider_filter_ = std::make_unique<KalmanFilter1D>(
            0.5f, 
            slider_params // 直接传递结构体
    );
        std::cout << "[SUCCESS] Kalman滤波器初始化完成\n";
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Kalman滤波器初始化失败: " << e.what() << std::endl;
        throw;
    }
}
float CameramanModel::predict(const std::vector<Point>& players, 
                            const std::vector<Point>& balls) {
    if (ConfigManager::ReloadCourtPointsIfNeeded()) {
        // 更新边界
        const auto& court_points = ConfigManager::Get().court_points;
        if (!court_points.empty()) {
            auto x_comparator = [](const Point& a, const Point& b) { return a.x < b.x; };
            auto min_it = std::min_element(court_points.begin(), court_points.end(), x_comparator);
            auto max_it = std::max_element(court_points.begin(), court_points.end(), x_comparator);
            left_most_ = min_it->x;
            right_most_ = max_it->x;
        }
    }
    std::cout << "\n==== 开始预测 ====\n";
    std::cout << "输入球员数量: " << players.size() 
              << ", 球数量: " << balls.size() << std::endl;

    auto processed_players = players;
    auto processed_balls = balls;
    handleEmptyInput(processed_players, processed_balls);

    std::cout << "处理后球员数量: " << processed_players.size()
              << ", 球数量: " << processed_balls.size() << std::endl;

    if (!initialized_) {
        std::cout << "首次运行，初始化历史数据...\n";
        try {
            initializeHistory(processed_players);
            initialized_ = true;
            std::cout << "历史数据初始化完成，队列大小: " 
                      << player_pos_memory_.size() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "历史数据初始化失败: " << e.what() << std::endl;
            throw;
        }
    }

    // 核心计算逻辑
    const float mean_pos = std::accumulate(processed_players.begin(), processed_players.end(), 0.0f,
        [](float sum, const Point& p) { return sum + p.x; }) / processed_players.size();

    // 更新记忆队列
    player_pos_memory_.push_back(mean_pos);
    player_max_memory_.push_back(std::max_element(processed_players.begin(), processed_players.end(),
        [](const Point& a, const Point& b) { return a.x < b.x; })->x);
    player_min_memory_.push_back(std::min_element(processed_players.begin(), processed_players.end(),
        [](const Point& a, const Point& b) { return a.x < b.x; })->x);

    // 计算速度
    const float speed = std::clamp(
        calculateAccumulatedSpeed(player_pos_memory_),
        -params_.camera.speed_max,
        params_.camera.speed_max
    );

    // 更新滑动参数
    const float slider = 0.5f * (speed / params_.camera.speed_max + 1.0f);
    const float filtered_slider = slider_filter_->filterMeasurement(slider);

    // 计算最终目标位置
    const float target_x = std::clamp(
        params_.camera.position_merge_ratio * mean_pos + 
        (1 - params_.camera.position_merge_ratio) * processed_balls[0].x,
        left_most_ - params_.camera.buffer_pixels,
        right_most_ + params_.camera.buffer_pixels
    );

    // 保存调试信息
    debug_info_.emplace(DebugInfo{
        target_x, 
        target_x,  // 实际应添加滤波处理
        mean_pos,
        speed,
        filtered_slider
    });

    return target_x;
}

std::tuple<float, float> CameramanModel::transfer(float x) const {
    // 读取参数
    const auto& t = params_.transfer;
    // 归一化
    float norm = (x - t.x_min) / (t.x_max - t.x_min);
    norm = std::clamp(norm, 0.0f, 1.0f);

    // y: 二次函数，中心最大，两端最小
    float y = t.y_min + (t.y_max - t.y_min) * (1 - std::pow(2 * norm - 1, 2));
    // fov: 二次函数，中心最大，两端最小
    float fov = t.fov_min + (t.fov_max - t.fov_min) * (1 - std::pow(2 * norm - 1, 2));
    return {y, fov};
}

// 其他成员函数实现...