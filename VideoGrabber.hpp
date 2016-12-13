#ifndef VIDEOGRABBER_HPP_
#define VIDEOGRABBER_HPP_

#include <opencv2/opencv.hpp>

using namespace cv;

class VideoGrabber {
public:
    enum Result {
        SUCCESS,
        END_OF_FILE,
        FILE_NOT_OPENED,
    };

    VideoGrabber();
    ~VideoGrabber();

    bool open(const char *filepath);
    void close();
    void setMaxSize(int width, int height);
    Result getNextFrame(Mat &frame);
    void seek(int time_millis, bool absolute = false);
    int getFrameRate();
    int getCurrentPosition();

private:
    VideoCapture capture;
    Mat work;

    int max_width;
    int max_height;
};

#endif /* VIDEOGRABBER_HPP_ */
