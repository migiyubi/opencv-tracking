#include <opencv2/highgui.hpp>
#include <opencv2/tracking.hpp>
#include <time.h>

#include "Gui.hpp"
#include "VideoGrabber.hpp"

using namespace cv;

static bool isValidRect(const Rect2d &rect) {
    return rect.width > 0.0 && rect.height > 0.0;
}

static long long getCurrentTimeMillis() {
    clock_t t = clock();
    return t * 1e3 / CLOCKS_PER_SEC;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        printf("Usage: %s source_filepath [mask_filepath [tracking_algorithm]]\n", argv[0]);
        printf("  available tracking algorithms are below\n");
        printf("    KCF(default), MEDIANFLOW, MIL, BOOSTING, TLD\n");
        return -1;
    }

    const char *src_filepath = argv[1];
    const char *mask_filepath = (argc >= 3) ? argv[2] : nullptr;
    const char *tracking_algorithm = (argc >= 4) ? argv[3] : "KCF";

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
    Rect2d roi_tracking, result_tracking;
    Ptr<Tracker> tracker;

    // first frame.
    grabber.getNextFrame(src_frame);
    gui.open(src_frame);

    if (mask_filepath != nullptr) {
        gui.setMaskImage(imread(mask_filepath, IMREAD_UNCHANGED));
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
                result_tracking.width = roi_tracking.width;
                result_tracking.height = roi_tracking.height;
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

            if (isValidRect(roi_tracking)) {
                tracker = Tracker::create(tracking_algorithm);
                tracking = tracker->init(src_frame, roi_tracking);
            }
            else {
                tracking = false;
                located = false;
            }
        }
        else if (key_lower == 0x000a || key_lower == 0xff8d) { // Enter
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
