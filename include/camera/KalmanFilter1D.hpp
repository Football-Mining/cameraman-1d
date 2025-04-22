// KalmanFilter1D.hpp
#pragma once
#include <Eigen/Dense>
#include "ConfigManager.hpp"

class KalmanFilter1D {
public:
    explicit KalmanFilter1D(
        float initial_x,
        const ConfigManager::Params::KalmanParams& params
    );
    float filterMeasurement(float measurement);

private:
    void predict();
    void update(float measurement);

    Eigen::MatrixXf F_;
    Eigen::MatrixXf H_;
    Eigen::MatrixXf P_;
    Eigen::MatrixXf Q_;
    Eigen::MatrixXf R_;
    Eigen::VectorXf x_;
};