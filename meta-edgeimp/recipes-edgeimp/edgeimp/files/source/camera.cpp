/* Edge Impulse Linux SDK
 * Copyright (c) 2024 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "opencv2/opencv.hpp"
#include "opencv2/videoio/videoio_c.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include <unistd.h>

#define IMAGE_FILE_PATH "/home/root/output/Image-"
#define DATA_FILE_PATH "/home/root/output/Data-"

using namespace std;
using namespace cv;

static float maxValue = 0.0;
static float probValue = 0.50;
static int codec = VideoWriter::fourcc('x', '2', '6', '4');
static float features[EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT];
cv::Mat frame;
cv::Mat infFrame;
cv::Mat croppedImg;
int camHeight;
int camWidth;
ei_impulse_result_t result;
signal_t signalFrame;
ei_impulse_result_bounding_box_t newBox;

class sysTime {
private:
  string epoch;
  string timenow;

public:
  void doTimeMeasure();
  string getCurrentTime();
  string getEpoch();
};

void sysTime::doTimeMeasure() {
  const auto now = std::chrono::system_clock::now();
  const auto duration = now.time_since_epoch();
  const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
  const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()) %
                     1000;
  const auto epoch_time =
      std::chrono::duration_cast<std::chrono::seconds>(duration).count();

  std::stringstream epochS;
  epochS << epoch_time;

  std::stringstream nowSs;
  nowSs << std::put_time(std::localtime(&nowAsTimeT), "%a:%b:%d:%Y,%T") << '.'
        << std::setfill('0') << std::setw(3) << nowMs.count();

  epoch = epochS.str();
  timenow = nowSs.str();
}

string sysTime::getEpoch() { return epoch; }
string sysTime::getCurrentTime() { return timenow; }
void scaleBoxes(ei_impulse_result_bounding_box_t *inBox) {

  // convert yolo xyhw to x1y1x2y2
  float x1 = (inBox->x);
  float y1 = (inBox->y);
  float x2 = (inBox->x + inBox->width);
  float y2 = (inBox->y + inBox->height);

  int ox1 = int((x1 / 256) * camWidth);
  int oy1 = int((y1 / 256) * camHeight);
  int ox2 = int((x2 / 256) * camWidth);
  int oy2 = int((y2 / 256) * camHeight);

  // convert back to xywh
  inBox->x = ox1;
  inBox->y = oy1;
  inBox->width = abs(ox2 - ox1);
  inBox->height = abs(oy2 - oy1);

  cout << "INF_Info: Rescaled Bounding box: x=" << inBox->x << " y=" << inBox->y
       << " w=" << inBox->width << " h=" << inBox->height << endl;
}

int main(int argc, char **argv) {

  /* Parse command line inputs */
  if (argc < 3) {
    printf("Requires two parameters <camera ID> <bowl ID>.\n");
    printf("You can find camera detials via `v4l2-ctl --list-devices`.\n");
    printf("E.g. for <camera ID>:\n");
    printf("          0 for /dev/video0\n");
    printf("          1 for /dev/video1\n");
    printf("E.g. for <bowl ID>:\n");
    printf("            D1\n");
    printf("            C1\n");
    printf("./edgeimp 0 D1\n");
    exit(1);
  }

  int count = 0;        // no of frames processed
  std::fstream outfile; // to save result data
  sysTime timeNow;      // system time
  string maxImgName;    // to save max image name with time stamp
  int maxCount = 5; // limit first 5 images with good results cropped & saved

  std::string deviceID(argv[2]);
  int cameraID = atoi(argv[1]);
  cv::VideoCapture camera;
  /* Camera Initialization */
  if (cameraID == 0) { /* Connect to camera 1 /dev/video0 */
    camera = cv::VideoCapture(
        "v4l2src device=/dev/video0 ! queue ! videoconvert ! appsink");
  } else if (cameraID == 1) {
    /* Connect to camera 1 /dev/video0 */
    camera = cv::VideoCapture(
        "v4l2src device=/dev/video1 ! queue ! videoconvert ! appsink");
  } else {
    printf("INF_Err: Error Camera divice not available\n");
    return -1;
  }
  if (!camera.isOpened()) {
    printf("INF_Err: Error can't create video capture\n");
    return -1;
  }

  /* Read Camera config */
  camWidth = camera.get(cv::CAP_PROP_FRAME_WIDTH);
  camHeight = camera.get(cv::CAP_PROP_FRAME_HEIGHT);
  cout << "INF_Info: Camera configured to HxW: " << camHeight << camWidth
       << endl
       << endl;

  // display the frame until you press a key
  while (1) {
    // capture the next frame from the webcam
    camera >> frame;
    count++;

    if (count % 4 == 0 && maxCount > 0) {

      // Measure time when you capture a frame
      timeNow.doTimeMeasure();

      // Pre-processing start
      cv::resize(frame, infFrame, cv::Size(256, 256));
      size_t feature_ix = 0;
      for (int rx = 0; rx < (int)infFrame.rows; rx++) {
        for (int cx = 0; cx < (int)infFrame.cols; cx++) {
          cv::Vec3b pixel = infFrame.at<cv::Vec3b>(rx, cx);
          uint8_t b = pixel.val[0];
          uint8_t g = pixel.val[1];
          uint8_t r = pixel.val[2];
          features[feature_ix++] = (r << 16) + (g << 8) + b;
        }
      }

      // construct a signal from the features buffer
      numpy::signal_from_buffer(
          features, EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT,
          &signalFrame);
      // Pre-processing end

      // Lib-funtionality start
      EI_IMPULSE_ERROR res = run_classifier(&signalFrame, &result, false);
      if (res != 0) {
        printf("INF_Err: Error failed to run classifier (%d)\n", res);
        return -1;
      }
      printf("INF_Info: Timing- DSP %d ms, inference %d ms, anomaly %d ms\r\n",
             result.timing.dsp, result.timing.classification,
             result.timing.anomaly);
      bool found_bb = false;
      // Lib-funtionality end

      // Process the results obtained - Post processing
      for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
        auto bb = result.bounding_boxes[ix];
        // If not detection or poor detection no processing needed
        if (bb.value == 0) {
          continue;
        }
        found_bb = true;
        cout << "INF_Info: Time epoch- " << timeNow.getCurrentTime() << endl;
        printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n",
               bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);

        if (probValue <= bb.value && maxCount > 0) {
          // Best 5 max values to be saved as cropped images
          maxCount--;

          newBox.x = bb.x;
          newBox.y = bb.y;
          newBox.width = bb.width;
          newBox.height = bb.height;
          scaleBoxes(&newBox);
          cv::Rect myROI(newBox.x, newBox.y, newBox.width, newBox.height);
          cout << "New Bounding box: x=" << newBox.x << ",y=" << newBox.y
               << ",w=" << newBox.width << ",h=" << newBox.height << endl;

          std::string cropImgname = IMAGE_FILE_PATH + deviceID + "-" +
                                    timeNow.getEpoch() + "_Crop.jpg";
          frame(myROI).copyTo(croppedImg);
          imwrite(cropImgname, croppedImg);
          croppedImg.release();
          if (!outfile.is_open()) {
            std::string datafile = DATA_FILE_PATH + deviceID + "-" +
                                   timeNow.getEpoch() + "_Infer.txt";
            outfile.open(datafile, ios::app);
            if (!outfile.is_open()) {
              cout << "INF_Err: Error No datafile to save the results" << endl;
              return -1;
            }
            outfile << "Time stamp: " << timeNow.getCurrentTime() << endl;
            outfile << "\tLabel: " << bb.label << endl;
            outfile << "\tValue: " << bb.value << endl;
            outfile << "\tBounding box: x=" << bb.x << ",y=" << bb.y
                    << ",w=" << bb.width << ",h=" << bb.height << endl;
            outfile.close();
          }
        }
      }
      if (!found_bb) {
        printf("INF_Info: no objects found\n");
      }
      infFrame.release();
    }

    // Stop criteria to be decided based on other sensors
    if (count > 800) {
      cout << "INF_Info: Processing of 200 frames completed - Calling Reset"
           << endl;
      // reset values
      maxValue = 0.0;
      maxCount = 5;
      count = 0;
    }
    frame.release();
  }
  return 0;
}
