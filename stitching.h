#ifndef STITCHING_H_
#define STITCHING_H_

#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>

namespace pp{
class Var;
class VarArray;
class VarDictionary;
}

// This class is used by Stitching to send string messages back to JS side.
// It is set into Stitching class via SetMessageHandler, and normaly it'll be
// implemented by the owner of it.
class MessageDispatcher {
 public:
  virtual void SendMessage(std::string msg) = 0;
  virtual void SendMessage(pp::VarDictionary dic) = 0;
};

class Stitching{
 public:
  Stitching() {}
  virtual ~Stitching() {}

  void InitializeOpenCV(int width, int height);

  // Calculate depth map based on two input images. InitialiseOpenCV() must
  // have been called previously.
  void CalculateDepthMap(cv::Mat img1, cv::Mat img2);

  void SendBackSineWave();

  const char* GetOpenCVVersion() const { return CV_VERSION; }

  const void SetMessageHandler(MessageDispatcher* handler) {
    msg_handler_ = handler;
  }
  const void SetImageData(
      int idx, int height, int width, const unsigned char* array) ;

 private:
  // maps for (left and right) camera rectification
  cv::Mat right_maps_[2];
  cv::Mat left_maps_[2];

  // block matching object
  cv::StereoSGBM bm_;

  MessageDispatcher* msg_handler_;
};


#endif  // STITCHING_H_
