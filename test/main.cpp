#include "camera/CameramanModel.hpp"
#include <iostream>
#include <vector>

int main() {
    try {
        ConfigManager::Initialize("../config/camera_config.json");

        // 直接用ConfigManager里的court_points
        CameramanModel model(ConfigManager::Get().court_points);

        // 模拟输入数据
        std::vector<Point> players = {{500, 300}, {600, 400}};
        std::vector<Point> balls = {{550, 350}};
        {
            float target = model.predict(players, balls);
            auto [y, fov] = model.transfer(target);
            std::cout << "Predicted Camera Target X: " << target << std::endl;
            std::cout << "Transfer result: Y = " << y << ", FOV = " << fov << std::endl;
        }
        players = {{600, 300}, {700, 400}};
        balls = {{570, 350}};
        {
            float target = model.predict(players, balls);
            auto [y, fov] = model.transfer(target);
            std::cout << "Predicted Camera Target X: " << target << std::endl;
            std::cout << "Transfer result: Y = " << y << ", FOV = " << fov << std::endl;
        }
        players = {{700, 300}, {800, 400}};
        balls = {{580, 350}};
        {
            float target = model.predict(players, balls);
            auto [y, fov] = model.transfer(target);
            std::cout << "Predicted Camera Target X: " << target << std::endl;
            std::cout << "Transfer result: Y = " << y << ", FOV = " << fov << std::endl;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}