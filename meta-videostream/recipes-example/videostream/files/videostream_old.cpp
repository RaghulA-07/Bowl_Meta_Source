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

static bool use_debug = false;
VideoWriter infwriter;
static Point text_position(15, 30);
static float fontSize = 1;
static Scalar fontColor(165, 42, 42);
static double fps = 30.0;
static Size sizeFrame(256, 256);
static int codec = VideoWriter::fourcc('x', '2', '6', '4');
cv::Mat frame, outFrame;
int camHeight;
int camWidth;

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

int main(int argc, char **argv) {

  /* Parse command line inputs */
  if (argc < 2) {
    printf("Requires one parameter (ID of the webcam).\n");
    printf("You can find camera detials via `v4l2-ctl --list-devices`.\n");
    printf("E.g. for:\n");
    printf("    C922 Pro Stream Webcam (usb-70090000.xusb-2.1):\n");
    printf("            /dev/video0\n");
    printf("The ID of the webcam is 0\n");
    printf("./build/camera 0 \n");
    exit(1);
  }

  for (int ix = 2; ix < argc; ix++) {
    if (strcmp(argv[ix], "--debug") == 0) {
      printf("Enabling debug mode\n");
      use_debug = true;
    }
  }

  int count = 0;   // no of frames processed
  sysTime timeNow; // system time

  /* Camera Initialization */
  cv::VideoCapture camera("v4l2src ! queue ! videoconvert ! appsink");
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

  if (use_debug) {
    // create a window to display the images from the webcam
    cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);
  }

  // display the frame until you press a key
  while (1) {
    // capture the next frame from the webcam
    camera >> frame;
    count++;
    // Measure time when you capture a frame
    timeNow.doTimeMeasure();

    std::string tag = "PetBowl-" + timeNow.getCurrentTime();
    putText(frame, tag, text_position, cv::FONT_HERSHEY_COMPLEX, fontSize,
            fontColor, 1);
    cout << endl << endl << "INF_Info: Starting recording" << endl;

    std::string pipeline =
        "appsrc ! queue ! videoconvert ! x264enc ! flvmux "
        "streamable=true ! queue ! rtmpsink "
        "location=rtmp://nginx-rtmp-pet.eastus.azurecontainer.io/live/"
        "D15-" +
        timeNow.getEpoch();
    cout << "INF_Info: " << pipeline << endl;
    infwriter.open(pipeline, codec, fps, cv::Size(640, 480), true);

    if (!infwriter.isOpened()) {
      cout << "INF_ERR: Error No writer to save the stream" << endl;
      return -1;
    }

    // continue to upload the frames
    cout << "INF_Info: Upload frame no- " << count << endl;
    cv::resize(frame, outFrame, cv::Size(640, 480));
    infwriter.write(outFrame);
    // show the image on the window
    if (use_debug) {
      cv::imshow("Webcam", outFrame);
      // wait (10ms) for a key to be pressed
      if (cv::waitKey(10) >= 0)
        break;
    }
  }
  frame.release();
  outFrame.release();
  infwriter.release();
  return 0;
}
