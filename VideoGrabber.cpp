#include "VideoGrabber.hpp"

VideoGrabber::VideoGrabber() {
    max_width = 0;
    max_height = 0;
}

VideoGrabber::~VideoGrabber() {
}

bool VideoGrabber::open(const char *filepath) {
    VideoCapture capture(filepath);

    if (!capture.isOpened()) {
        return false;
    }

    this->capture = capture;

    return true;
}

void VideoGrabber::close() {
    if (!capture.isOpened()) {
        capture.release();
    }
}

void VideoGrabber::setMaxSize(int width, int height) {
    max_width = width;
    max_height = height;
}

VideoGrabber::Result VideoGrabber::getNextFrame(Mat &frame) {
    if (!capture.isOpened()) {
        return FILE_NOT_OPENED;
    }

    double scale = 1.0;

    if (max_width > 0 && max_height > 0) {
        double scale_x = max_width / capture.get(CV_CAP_PROP_FRAME_WIDTH);
        double scale_y = max_height / capture.get(CV_CAP_PROP_FRAME_HEIGHT);
        scale = min(min(scale_x, scale_y), 1.0);
    }

    if (!capture.read(work)) {
        return END_OF_FILE;
    }

    resize(work, frame, Size(), scale, scale);

    return SUCCESS;
}

void VideoGrabber::seek(int time_millis, bool absolute) {
    if (!absolute) {
        time_millis += capture.get(CV_CAP_PROP_POS_MSEC);
    }

    capture.set(CV_CAP_PROP_POS_MSEC, time_millis);
}

int VideoGrabber::getFrameRate() {
    return static_cast<int>(capture.get(CV_CAP_PROP_FPS));
}

int VideoGrabber::getCurrentPosition() {
    return static_cast<int>(capture.get(CV_CAP_PROP_POS_FRAMES));
}
