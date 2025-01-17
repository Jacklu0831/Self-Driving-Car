#include "FusionEKF.h"
#include <iostream>
#include "Eigen/Dense"
#include "tools.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::cout;
using std::endl;
using std::vector;

// Constructor
FusionEKF::FusionEKF() {
  is_initialized_ = false;
  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  // measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0, 0.0225;

  // measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;

  // measurement matrix - laser
  H_laser_ << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0;

  // jacobian matrix - radar
  Hj_ << 0, 0, 0, 0,
         0, 0, 0, 0,
         0, 0, 0, 0;

  // measurement noises - both
  noise_ax = 9.0;
  noise_ay = 9.0;
}


// Destructor
FusionEKF::~FusionEKF() {}


void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
  /**
   * Initialization
   */
  if (!is_initialized_) {

    // first measurement
    VectorXd x_in = VectorXd(4);
    x_in << 1, 1, 1, 1;
  
    // state covariance matrix
    MatrixXd P_in = MatrixXd(4, 4);
    P_in << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1000, 0,
            0, 0, 0, 1000;
    // initial transition matrix
    MatrixXd F_in = MatrixXd(4, 4);
    F_in << 1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1;  

    MatrixXd Q_in = MatrixXd(4,4);
    Q_in << 1, 0, 1, 0,
           0, 1, 0, 1,
           1, 0, 1, 0,
           0, 1, 0, 1;

    MatrixXd H_in;
    MatrixXd R_in;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {

      // convert radar from polar to cartesian coordinates and initialize state.
      x_in << measurement_pack.raw_measurements_[0] * cos(measurement_pack.raw_measurements_[1]),
              measurement_pack.raw_measurements_[0] * sin(measurement_pack.raw_measurements_[1]),
              0, 0;

      H_in = Hj_;
      R_in = R_radar_;
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      // initialize state.
      x_in << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0, 0;
      H_in = H_laser_;
      R_in = R_laser_;
    }

    ekf_.Init(x_in, P_in, F_in, H_in, R_in, Q_in);
    // done initializing
    is_initialized_ = true;
    previous_timestamp_ = measurement_pack.timestamp_;
    return;
  }

  /**
   * Prediction
   */

  // compute the time elapsed between the current and previous measurements (seconds)
  float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;
  previous_timestamp_ = measurement_pack.timestamp_;

  // store temporary values for later
  float dt_2 = dt * dt;
  float dt_3 = dt_2 * dt;
  float dt_4 = dt_3 * dt;

  // integrate time into F matrix
  ekf_.F_(0, 2) = dt;
  ekf_.F_(1, 3) = dt;

  // set the process covariance matrix Q
  ekf_.Q_ <<  dt_4 / 4 * noise_ax, 0, dt_3 / 2 * noise_ax, 0,
              0, dt_4 / 4 * noise_ay, 0, dt_3 / 2 * noise_ay,
              dt_3 / 2 * noise_ax, 0, dt_2 * noise_ax, 0,
              0, dt_3 / 2 * noise_ay, 0, dt_2 * noise_ay;

  ekf_.Predict();

  /**
   * Update
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    ekf_.R_ = R_radar_;
    ekf_.H_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
    ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}

