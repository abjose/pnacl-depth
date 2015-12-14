#include "opencv2/calib3d/calib3d.hpp"
//#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

using namespace cv;
using namespace std;

int main(int argc, char** argv) {
  float veryBig = 100000;
  float verySmall = 1e-5;
  const float squareSize = 2.4f;  // cm
  VideoCapture video_l(0);
  VideoCapture video_r(1);
  video_l.set(CV_CAP_PROP_FRAME_WIDTH, 320);
  video_l.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
  video_r.set(CV_CAP_PROP_FRAME_WIDTH, 320);
  video_r.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
  Mat R, T, E, F;
  Mat frame_l, frame_r;
  int framenumber = 0;
  int board_dt = 500;
  int board_w = 6;
  int board_h = 9;
  Size boardSize = Size(board_w, board_h);
  Mat frame_l_grey, frame_r_grey;
  Mat cameraMatrix_l = Mat::eye(3, 3, CV_64F);
  Mat cameraMatrix_r = Mat::eye(3, 3, CV_64F);
  Mat distCoeffs_l = Mat::zeros(8, 1, CV_64F);
  Mat distCoeffs_r = Mat::zeros(8, 1, CV_64F);
  vector<Mat> rvecs_l, tvecs_l;
  vector<Mat> rvecs_r, tvecs_r;
  Mat rectify_l, rectify_r, projection_l, projection_r, Q;

  int successes=0;
  int n_boards=5;

  //vector<vector<Point2f> > cornervector_l, cornervector_r;
  vector<vector<Point2f> > imagePoints_l(n_boards);
  vector<vector<Point2f> > imagePoints_r(n_boards);
  vector<vector<Point3f> > objectPoints(n_boards);

  if (!video_l.isOpened()) {
      cout << "Couldn't open camera 0!" << endl;
      return -1;   
  }
  if (!video_r.isOpened()) {
      cout << "Couldn't open camera 1!" << endl;
      return -1;   
  }
  cout << "Press space to capture once checkerboard is in view." << endl;

  Size imageSize = Size(video_r.get(CV_CAP_PROP_FRAME_WIDTH), 
                        video_r.get(CV_CAP_PROP_FRAME_HEIGHT));

  namedWindow("Webcaml", CV_WINDOW_AUTOSIZE);
  namedWindow("Webcamr", CV_WINDOW_AUTOSIZE);
  while(successes < n_boards) {
    video_l.read(frame_l);
    video_r.read(frame_r);
    //if(false && framenumber++ % board_dt == 0 && framenumber != 0) {
    vector<Point2f>& points_l = imagePoints_l[successes];
    vector<Point2f>& points_r = imagePoints_r[successes];
    //vector<Point2f> points_l, points-R;
    //bool patternfoundl = findChessboardCorners(frame_l, boardSize, corners_l);
    bool patternfoundl = findChessboardCorners(frame_l, boardSize, points_l,
                                               CV_CALIB_CB_FILTER_QUADS +
                                               CV_CALIB_CB_ADAPTIVE_THRESH +
                                               CALIB_CB_FAST_CHECK);
    //bool patternfoundr = findChessboardCorners(frame_r, boardSize, corners_r);
    bool patternfoundr = findChessboardCorners(frame_r, boardSize, points_r,
                                               CV_CALIB_CB_FILTER_QUADS +
                                               CV_CALIB_CB_ADAPTIVE_THRESH +
                                               CALIB_CB_FAST_CHECK);

    string msg = format("%d/%d", (int) successes, n_boards);
    if(patternfoundl && patternfoundr) {
      cvtColor(frame_l, frame_l_grey, CV_RGB2GRAY);
      cvtColor(frame_r, frame_r_grey, CV_RGB2GRAY);

      cornerSubPix(frame_l_grey, points_l, Size(6,6), Size(-1,-1),
                   TermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER,
                                veryBig, verySmall));
      cornerSubPix(frame_r_grey, points_r, Size(6,6), Size(-1,-1),
                   TermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER,
                                veryBig, verySmall));
      drawChessboardCorners(frame_l, boardSize, points_l, patternfoundl);
      drawChessboardCorners(frame_r, boardSize, points_r, patternfoundr);

      char c = cvWaitKey(1);
      if(c == ' ') {
        cout << "capturing!" << endl;
        ++successes;
      }
    }

    int baseLine = 0;
    Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
    Point textOrigin(frame_r.cols - 2*textSize.width - 10, frame_r.rows - 2*baseLine - 10);
    putText(frame_r, msg, textOrigin, 1, 1, Scalar(0,255,0));

    imshow("Webcaml", frame_l);
    imshow("Webcamr", frame_r);

    char c = cvWaitKey(1);
    if(c == 27) break;
  }
  destroyAllWindows();
  // populate objectPoints; because using same pattern each time all entries
  // are the same
  for(int i = 0; i < n_boards; i++)
    for(int j = 0; j < boardSize.height; j++)
      for(int k = 0; k < boardSize.width; k++)
        objectPoints[i].push_back(Point3f(k*squareSize, j*squareSize, 0));

  // calibrate each camera seperately
  double rms_l, rms_r;
  rms_l = calibrateCamera(objectPoints, imagePoints_l, imageSize,
                          cameraMatrix_l, distCoeffs_l, rvecs_l, tvecs_l,
                          CV_CALIB_FIX_K3|CV_CALIB_FIX_K4|CV_CALIB_FIX_K5,
                          cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,
                                         veryBig, verySmall));
  rms_r = calibrateCamera(objectPoints, imagePoints_r, imageSize,
                          cameraMatrix_r, distCoeffs_r, rvecs_r, tvecs_r,
                          CV_CALIB_FIX_K3|CV_CALIB_FIX_K4|CV_CALIB_FIX_K5,
                          cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,
                                         veryBig, verySmall));
  cout << "intrinsic_l = " << cameraMatrix_l << endl;
  cout << "intrinsic_r = " << cameraMatrix_r << endl;
  cout << "distCoeffs_l = " << distCoeffs_l << endl;
  cout << "distCoeffs_r = " << distCoeffs_r << endl;
  cout << "tvecs_l = " << tvecs_l[0] << endl;
  cout << "rvecs_l = " << rvecs_l[0] << endl;
  double rms_stereo = 
    stereoCalibrate(objectPoints, imagePoints_l, imagePoints_r,
                    //point3dvector, cornervector_l, cornervector_r, 
                    cameraMatrix_l, distCoeffs_l, cameraMatrix_r, distCoeffs_r,
                    imageSize, R, T, E, F);
                    // TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, veryBig, verySmall),
                    // CV_CALIB_FIX_K3+CV_CALIB_FIX_INTRINSIC);

  Rodrigues(R, R);
  // cout << "R = " << R << endl;
  // cout << "T = " << T << endl;
  // cout << "E = " << E << endl;
  // cout << "F = " << F << endl;
  cout << "RMS l: " << rms_l << endl;
  cout << "RMS r: " << rms_r << endl;
  cout << "RMS stereo: " << rms_stereo << endl;
  stereoRectify(cameraMatrix_l, distCoeffs_l, cameraMatrix_r, distCoeffs_r,
                imageSize,
                R, T, rectify_l, rectify_r, projection_l, projection_r, Q);

  cout << "rectify_l = " << rectify_l << endl;
  cout << "rectify_r = " << rectify_r << endl;
  cout << "projection_l = " << projection_l << endl;
  cout << "projection_r = " << projection_r << endl;

  cout << "preparing to undistort" << endl;

  cv::Mat right_maps[2];
  cv::Mat left_maps[2];
  initUndistortRectifyMap(cameraMatrix_l, distCoeffs_l, rectify_l, projection_l,
                          imageSize, CV_16SC2, left_maps[0], left_maps[1]);
  initUndistortRectifyMap(cameraMatrix_r, distCoeffs_r, rectify_r, projection_r,
                          imageSize, CV_16SC2, right_maps[0], right_maps[1]);

  cout << "remapping" << endl;

  Mat canvas;
  double sf;
  int w, h;
  sf = 600./MAX(imageSize.width, imageSize.height);
  w = cvRound(imageSize.width*sf);
  h = cvRound(imageSize.height*sf);
  canvas.create(h, w*2, CV_8UC3);
  Mat rframe_l, rframe_r, cimg_l, cimg_r;
  //cv::Mat disparity(imageSize, CV_16SC1);

  /*
  int minDisparity = 0;
  int numDisparities = 64;
  int SADWindowSize = 5;
  Ptr<StereoSGBM> sgbm = StereoSGBM::create(minDisparity, numDisparities,
                                            SADWindowSize);
  // cv::StereoSGBM bm = cv::StereoSGBM(minDisparity, numDisparities, 
  //                                    SADWindowSize);
  //StereoSGBM bm(minDisparity, numDisparities, SADWindowSize);
  //StereoSGBM bm();

  sgbm->setPreFilterCap(63);
  int sgbmWinSize = SADWindowSize > 0 ? SADWindowSize : 3;
  sgbm->setBlockSize(sgbmWinSize);

  // int cn = img1.channels();
  int cn = frame_l.channels();
  sgbm->setP1(8*cn*sgbmWinSize*sgbmWinSize);
  sgbm->setP2(32*cn*sgbmWinSize*sgbmWinSize);
  sgbm->setMinDisparity(0);
  sgbm->setNumDisparities(numDisparities);
  sgbm->setUniquenessRatio(10);
  sgbm->setSpeckleWindowSize(100);
  sgbm->setSpeckleRange(32);
  sgbm->setDisp12MaxDiff(1);
  sgbm->setMode(StereoSGBM::MODE_SGBM);
  */

  // OK, why is disparity so grey?
  // type issue? scaling issue?
  // try just multiplying everything by something

  int num_channels = 1;
  int minDisparity = 4;
  int numDisparities = 96;
  int SADWindowSize = 3;
  int p1 = 8 * num_channels * SADWindowSize * SADWindowSize;
  int p2 = 32 * num_channels * SADWindowSize * SADWindowSize;
  int disp12MaxDiff = 1;
  int preFilterCap = 63;
  int uniquenessRatio = 10;
  int speckleWindowSize = 100;
  int speckleRange = 32;
  bool fullDP = false;
  cv::StereoSGBM sgbm = 
    cv::StereoSGBM(minDisparity, numDisparities, SADWindowSize,
                   p1, p2, disp12MaxDiff, preFilterCap, uniquenessRatio, 
                   speckleWindowSize, speckleRange, fullDP);
  cv::Mat disparity(imageSize, CV_16UC1);
  //sgbm(rectified_left, rectified_right, disparity);

  while(true) {
    video_l.read(frame_l);
    video_r.read(frame_r);

    remap(frame_l, rframe_l, left_maps[0], left_maps[1], cv::INTER_LINEAR);
    remap(frame_r, rframe_r, right_maps[0], right_maps[1], cv::INTER_LINEAR);
    // cvtColor(rframe_l, cimg_l, COLOR_GRAY2BGR);
    // cvtColor(rframe_r, cimg_r, COLOR_GRAY2BGR);
    Mat canvasPart_l = canvas(Rect(0, 0, w, h));
    Mat canvasPart_r = canvas(Rect(w, 0, w, h));
    // resize(cimg_l, canvasPart_l, canvasPart_l.size(), 0, 0, INTER_AREA);
    // resize(cimg_r, canvasPart_r, canvasPart_r.size(), 0, 0, INTER_AREA);
    resize(rframe_l, canvasPart_l, canvasPart_l.size(), 0, 0, INTER_AREA);
    resize(rframe_r, canvasPart_r, canvasPart_r.size(), 0, 0, INTER_AREA);

    // what does this do?
    // Rect vroi(cvRound(validRoi[k].x*sf), cvRound(validRoi[k].y*sf),
    //           cvRound(validRoi[k].width*sf), cvRound(validRoi[k].height*sf));
    // rectangle(canvasPart, vroi, Scalar(0,0,255), 3, 8);

    for(int j = 0; j < canvas.rows; j += 16)
      line(canvas, Point(0, j), Point(canvas.cols, j), Scalar(0, 255, 0), 1, 8);

    //sgbm->compute(rframe_l, rframe_r, disparity);
    sgbm(rframe_l, rframe_r, disparity);
    disparity *= 16;
    disparity.convertTo(disparity, CV_8UC1, 0.00390625);

    imshow("rectified", canvas);
    imshow("disparity", disparity);

    char c = cvWaitKey(1);
    if(c == 27) break;
  }
}
