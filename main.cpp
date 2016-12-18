#include <opencv2/highgui.hpp>
#include <opencv2/tracking.hpp>
#include <time.h>

#include "Gui.hpp"
#include "VideoGrabber.hpp"

using namespace cv;

static bool validateRects(std::vector<Rect2d> &rects) {
    std::vector<Rect2d>::iterator it = rects.begin();
    while (it != rects.end()) {
        if (it->width <= 0.0 || it->height <= 0.0) {
            rects.erase(it);
        }
        else {
            ++it;
        }
    }
    return rects.size() > 0;
}

static long long getCurrentTimeMillis() {
    clock_t t = clock();
    return t * 1e3 / CLOCKS_PER_SEC;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s source_filepath [mask_filepath ...]\n", argv[0]);
        return -1;
    }

    const char *src_filepath = argv[1];
    const char *tracking_algorithm = "KCF";

    VideoGrabber grabber;
    if (!grabber.open(src_filepath)) {
        printf("failed to open video. : %s\n", src_filepath);
        return -2;
    }
    grabber.setMaxSize(640, 360);

    printf("algorithm %s chosen\n", tracking_algorithm);

    Gui gui("Main");

    bool playing = true;
    bool tracking = false;
    bool located = false;
    bool debug = false;

    Mat src_frame;
    std::vector<Rect2d> roi_tracking, result_tracking;
    Ptr<MultiTracker> tracker;

    // first frame.
    grabber.getNextFrame(src_frame);
    gui.open(src_frame);

    for (int i = 2; i < argc; i++) {
        gui.addMaskImage(imread(argv[i], IMREAD_UNCHANGED));
    }

    long long t0 = getCurrentTimeMillis();
    int ideal_wait = 1000 / grabber.getFrameRate();

    while (true) {
        bool position_dirty = false;

        // track if the frame is updated.
        if (playing && tracking) {
            located = tracker->update(src_frame, result_tracking);

            // Note: somehow, KCF does not return detected size.
            //       set those of initial roi.
            if (located && strcmp("KCF", tracking_algorithm) == 0) {
                for (int i = 0; i < roi_tracking.size(); i++) {
                    result_tracking[i].width = roi_tracking[i].width;
                    result_tracking[i].height = roi_tracking[i].height;
                }
            }
        }

        // render.
        int frame_id = grabber.getCurrentPosition();
        gui.update(src_frame, result_tracking, located, frame_id, debug);

        // calculate time to wait.
        long long t1 = getCurrentTimeMillis();
        int wait = max(ideal_wait + (int)(t0 - t1), 1);
        t0 = t1;

        // handle key events.
        int key_code = waitKey(wait); // TODO: waitKey() may not have enough accuracy to stabilize frame rate. neither does clock().
        int key_upper = key_code >> 16;
        int key_lower = key_code & 0xffff;

        if (key_lower == 0x20) { // Space
            // set object to track.
            gui.selectRoi(roi_tracking);

            if (validateRects(roi_tracking)) {
                tracker = new MultiTracker(tracking_algorithm); // it seems there is no way to reset instantiated tracker.
                tracking = tracker->add(src_frame, roi_tracking);
            }
            else {
                tracking = false;
                located = false;
            }
        }
        else if (/*linux*/ key_lower == 0x000a || key_lower == 0xff8d || /*windows*/ key_lower == 0x000d) { // Enter
            // toggle play/pause.
            playing = !playing;
        }
        else if (key_lower == 0x001b) { // Esc
            // exit.
            break;
        }
        else if (key_upper == 0x0025 || key_lower == 0xff51) { // Left
            // rewind.
            grabber.seek(-10 * 1000);
            position_dirty = true;
        }
        else if (key_upper == 0x0027 || key_lower == 0xff53) { // Right
            // fast forward.
            grabber.seek(10 * 1000);
            position_dirty = true;
        }
        else if (key_lower == 0x0064 || key_lower == 0x0044) { // D
            // toggle debugging.
            debug = !debug;
        }

        // next frame.
        if (playing) {
            position_dirty = true;
        }

        if (position_dirty) {
            position_dirty = false;
            VideoGrabber::Result result = grabber.getNextFrame(src_frame);

            if (result == VideoGrabber::Result::END_OF_FILE) {
                // repeat from beginning.
                grabber.seek(0, true);
                grabber.getNextFrame(src_frame);
            }
        }
    }

    gui.close();
    grabber.close();

    return 0;
}
