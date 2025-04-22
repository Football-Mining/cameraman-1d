#include "camera/KalmanFilter1D.hpp"

KalmanFilter1D::KalmanFilter1D(float initial_x, const ConfigManager::Params::KalmanParams& params) {
    F_ = Eigen::MatrixXf::Identity(1, 1);
    H_ = Eigen::MatrixXf::Identity(1, 1);
    P_ = Eigen::MatrixXf::Identity(1, 1) * params.variance_position;
    Q_ = Eigen::MatrixXf::Identity(1, 1) * params.process_noise;
    R_ = Eigen::MatrixXf::Identity(1, 1) * params.variance_measurement;
    x_ = Eigen::VectorXf(1);
    x_ << initial_x;
}

float KalmanFilter1D::filterMeasurement(float measurement) {
    predict();
    Eigen::VectorXf z(1);
    z << measurement;
    Eigen::MatrixXf K = P_ * H_.transpose() * (H_ * P_ * H_.transpose() + R_).inverse();
    x_ += K * (z - H_ * x_);
    P_ = (Eigen::MatrixXf::Identity(1, 1) - K * H_) * P_;
    return x_(0);
}

void KalmanFilter1D::predict() {
    x_ = F_ * x_;
    P_ = F_ * P_ * F_.transpose() + Q_;
}