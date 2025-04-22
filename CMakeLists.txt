cmake_minimum_required(VERSION 3.15)
project(SmartCamera)

set(CMAKE_CXX_STANDARD 17)

# 手动指定头文件路径
include_directories(
    /opt/homebrew/include  # 关键修改：添加Homebrew头文件目录
    ${PROJECT_SOURCE_DIR}/include
)

# 手动指定Eigen路径（如果使用Homebrew安装）
find_package(Eigen3 REQUIRED HINTS /opt/homebrew/opt/eigen/share/eigen3/cmake)

add_library(camera_model STATIC
    src/camera/ConfigManager.cpp
    src/camera/KalmanFilter1D.cpp
    src/camera/CameramanModel.cpp
)

# 显式链接数学库（某些系统需要）
target_link_libraries(camera_model
    Eigen3::Eigen
    m  # 添加数学库链接
)

# 可执行文件
add_executable(main_test test/main.cpp)

target_link_libraries(main_test camera_model)
