#include "opencv2/opencv.hpp"
#include "opencv2/videoio/videoio_c.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include <unistd.h>

using namespace std;
using namespace cv;

static Point text_position(15, 30);
static float fontSize = 1;
static Scalar fontColor(0, 0, 0);
static double fps = 10.0;
static Size sizeFrame(256, 256);
static int codec = VideoWriter::fourcc('x', '2', '6', '4');
VideoWriter infwriter;
cv::Mat frame, outFrame;
int camHeight;
int camWidth;

class sysTime {
private:
  string epoch;
  string timenow;
  string firstEpoch;

public:
  void doTimeMeasure();
  string getCurrentTime();
  string getEpoch();
  string getCurrentEpoch();
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
  if (firstEpoch.empty())
    firstEpoch = epochS.str();
}
string sysTime::getEpoch() { return firstEpoch; }
string sysTime::getCurrentEpoch() { return epoch; }
string sysTime::getCurrentTime() { return timenow; }

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
    printf("./videostream 0 D1\n");
    exit(1);
  }

  int count = 0;   // no of frames processed
  sysTime timeNow; // system time
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
    // Measure time when you capture a frame
    timeNow.doTimeMeasure();

    std::string tag = "Bowl-" + timeNow.getCurrentTime();
    putText(frame, tag, text_position, cv::FONT_HERSHEY_SIMPLEX, fontSize,
            fontColor, 1);

    std::string pipeline =
        "appsrc ! queue ! videoconvert ! x264enc ! flvmux "
        "streamable=true ! queue ! rtmpsink "
        "location=rtmp://nginx-rtmp-pet.eastus.azurecontainer.io/live/" +
        deviceID + "-" + timeNow.getEpoch();
    if (count == 1) {
      cout << "INF_Info: Starting recording" << endl;
      cout << "INF_Info: " << pipeline << endl;
    }
    infwriter.open(pipeline, codec, fps, cv::Size(640, 480), true);

    if (!infwriter.isOpened()) {
      cout << "INF_ERR: Error No writer to save the stream" << endl;
      return -1;
    }

    // continue to upload the frames
    if (count % 10 == 0) {
      cout << "INF_Info: Uploading frame no- " << count << endl;
    }
    cv::resize(frame, outFrame, cv::Size(640, 480));
    infwriter.write(outFrame);
  }
  frame.release();
  outFrame.release();
  infwriter.release();
  return 0;
}
