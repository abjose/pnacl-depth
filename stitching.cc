#include "stitching.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <opencv2/imgproc/imgproc.hpp>  // cvtColor

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/cpp/var_dictionary.h"

namespace {

std::string print(int i) {
  char buffer[100];
  sprintf(buffer, "%d", i);
  return(std::string(buffer));
}

}

void Stitching::InitializeOpenCV(int width, int height) {
  cv::Size image_size = cv::Size(width, height);

  // For now the camera calibration is hard-coded. Run some stereo calibration
  // tool (like the one in the OpenCV examples) to update these values for
  // you cameras.

  // Precompute maps for cv::remap()
  cv::Mat left_intrinsics = (cv::Mat_<float>(3, 3) <<
                             274.6598681355454, 0, 169.0565490256286,
                             0, 274.8338549190667, 123.9549162468436,
                             0, 0, 1);
  cv::Mat left_distortion = (cv::Mat_<float>(5, 1) <<
                             0.006912663126351827,
                             -0.0904563977236823,
                             0.007800438105480427,
                             0.006192122851562848,
                             0);
  cv::Mat R1 = (cv::Mat_<float>(3, 3) <<
                0.998995905458896, -0.02074959927610454, 0.03970686346516869,
                0.01967704155880033, 0.9994359275200654, 0.02721471693036336,
                -0.04024916038694112, -0.02640607717908216, 0.9988406900883436);
  cv::Mat P1 = (cv::Mat_<float>(3, 4) <<
                273.4764288351131, 0, 151.5981407165527, 0,
                0, 273.4764288351131, 127.1885013580322, 0,
                0, 0, 1, 0);
  cv::Mat right_intrinsics = (cv::Mat_<float>(3, 3) <<
                              273.3233435937544, 0, 161.3729376092216,
                              0, 273.4764288351131, 125.8037714946607,
                              0, 0, 1);
  cv::Mat right_distortion = (cv::Mat_<float>(5, 1) <<
                              0.003586215459652761,
                              -0.110676926525934,
                              0.004329380048848819,
                              3.73975278964648e-05,
                              0);
  cv::Mat R2 = (cv::Mat_<float>(3, 3) <<
                0.9990469494367213, -0.009206058217049765, 0.04266662997337828,
                0.01034721500205531, 0.9995925333747614, -0.02660267586404496,
                -0.04240433896291902, 0.0270188029625757, 0.9987351282114729);
  cv::Mat P2 = (cv::Mat_<float>(3, 4) <<
                273.4764288351131, 0, 151.5981407165527, -2582.615310340987,
                0, 273.4764288351131, 127.1885013580322, 0,
                0, 0, 1, 0);

  initUndistortRectifyMap(left_intrinsics, left_distortion, R1, P1,
                          image_size, CV_16SC2, left_maps_[0], left_maps_[1]);
  initUndistortRectifyMap(right_intrinsics, right_distortion, R2, P2,
                          image_size, CV_16SC2, right_maps_[0], right_maps_[1]);

  // instantiate block matching class
  int num_channels = 4;
  //int minDisparity = 4;
  int minDisparity = 0;
  //int numDisparities = 96;
  int numDisparities = 64;
  int SADWindowSize = 3;
  int p1 = 8 * num_channels * SADWindowSize * SADWindowSize;
  int p2 = 32 * num_channels * SADWindowSize * SADWindowSize;
  int disp12MaxDiff = 1;
  int preFilterCap = 63;
  int uniquenessRatio = 10;
  int speckleWindowSize = 100;
  int speckleRange = 32;
  bool fullDP = false;
  bm_ = cv::StereoSGBM(minDisparity, numDisparities, SADWindowSize,
                       p1, p2, disp12MaxDiff, preFilterCap, uniquenessRatio,
                       speckleWindowSize, speckleRange, fullDP);
}

void Stitching::CalculateDepthMap(cv::Mat img1, cv::Mat img2) {

  SendSineWave(img1.rows, img1.cols);

  return;

  //bm->setPreFilterType(cv::StereoBM::PREFILTER_XSOBEL);

  cv::Mat rectified_left(img1.size(), CV_8UC4);
  cv::Mat rectified_right(img1.size(), CV_8UC4);
  remap(img1, rectified_left, left_maps_[0], left_maps_[1], cv::INTER_LINEAR);
  remap(img2, rectified_right, right_maps_[0], right_maps_[1], cv::INTER_LINEAR);

  cv::Mat disparity(img1.size(), CV_16SC1);
  cv::Mat disparity8(img1.size(), CV_8UC1);
  cv::Mat disparity8rgba(img1.size(), CV_8UC4);
  bm_(rectified_left, rectified_right, disparity);


  //disparity *= 16;
  //disparity.convertTo(disparity, CV_8UC1, 1/256);
  disparity.convertTo(disparity8, CV_8UC1);
  cvtColor(disparity8, disparity8rgba, CV_GRAY2RGBA);


  auto nBytes = img1.elemSize() * img1.total();
  pp::VarDictionary msg;
  pp::VarArrayBuffer data1(nBytes);
  pp::VarArrayBuffer data2(nBytes);
  uint8_t* copy1 = static_cast<uint8_t*>(data1.Map());
  uint8_t* copy2 = static_cast<uint8_t*>(data2.Map());
  memcpy(copy1, rectified_left.data, nBytes);
  memcpy(copy2, rectified_right.data, nBytes);

  msg.Set("message", "rectified");
  msg.Set("data1", data1);
  msg.Set("data2", data2);
  msg_handler_->SendMessage(msg);


  // send disparity
  //auto dBytes = disparity8.elemSize() * disparity8.total();
  pp::VarDictionary msg2;
  pp::VarArrayBuffer data3(nBytes);
  uint8_t* copy3 = static_cast<uint8_t*>(data3.Map());
  memcpy(copy3, disparity8rgba.data, nBytes);

  msg2.Set("message", "disparity");
  msg2.Set("data", data3);
  msg_handler_->SendMessage(msg2);
}

void SendSineWave(int rows, int cols) {

  cv::Mat sine(cols, rows, CV_8U(4));


  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      int c1 = 127*((1+sin(row/5)/2));
      //int c1 = 127*((1+sin(row/5)/2));
      int val = c1;

      sine.at<uchar>(row, col, 0) = val;
      sine.at<uchar>(row, col, 1) = val;
      sine.at<uchar>(row, col, 2) = val;
      sine.at<uchar>(row, col, 3) = 255;
    }
  }

  int nBytes = sine.elemSize() * sine.total();
  pp::VarDictionary msg;
  pp::VarArrayBuffer data(nBytes);
  uint8_t* copy = static_cast<uint8_t*>(data.Map());
  memcpy(copy, rectified_left.data, nBytes);

  msg.Set("message", "disparity");
  msg.Set("data", data);
  msg_handler_->SendMessage(msg);
}
