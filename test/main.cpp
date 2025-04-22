#include "camera/CameramanModel.hpp"
#include <iostream>
#include <vector>

int main() {
    try {
        ConfigManager::Initialize("../config/camera_config.json");
        
        // 测试用球场边界（全屏坐标）
        std::vector<Point> court = {{0, 0}, {1920, 1080}};
        
        CameramanModel model(court);
        
        // 模拟输入数据
        std::vector<Point> players = {{500, 300}, {600, 400}};
        std::vector<Point> balls = {{550, 350}};
        
        // 执行预测
        float target = model.predict(players, balls);
        
        std::cout << "Predicted Camera Target X: " << target << std::endl;

        players = {{600, 300}, {700, 400}};
        balls = {{570, 350}};
        target = model.predict(players, balls);
        std::cout << "Predicted Camera Target X: " << target << std::endl;

        players = {{700, 300}, {800, 400}};
        balls = {{580, 350}};
        target = model.predict(players, balls);
        std::cout << "Predicted Camera Target X: " << target << std::endl;
        
        // if (auto debug = model.getDebugInfo()) {
        //     std::cout << "Debug Info:\n"
        //               << "Raw Target: " << debug->raw_target << "\n"
        //               << "Filtered Target: " << debug->filtered_target << "\n"
        //               << "Mean Player Pos: " << debug->mean_player_pos << "\n"
        //               << "Calculated Speed: " << debug->calculated_speed << "\n"
        //               << "Focus Slider: " << debug->focus_slider << std::endl;
        // }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}