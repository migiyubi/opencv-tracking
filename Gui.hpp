#ifndef GUI_HPP_
#define GUI_HPP_

#include <opencv2/highgui.hpp>

using namespace cv;

class Gui {
public:
    Gui(const char *window_name, Point initial_position = Point(0, 0));
    ~Gui();

    void open(const Mat &frame);
    void update(const Mat &frame, const Rect2d &mask_rect, bool tracking, int frame_id, bool debug = false);
    void close();
    void selectRoi(Rect2d &roi);
    void setMaskImage(const Mat &image);

private:
    const char *window_name;
    Point initial_position;

    Mat current_frame_orig;
    Mat current_frame;
    Mat image_mask;

    void drawImage(const Mat &fore_image, Mat &base_image, const Mat &affine);
    void getLinearTransformMatrix(Mat &warp_mat, const Rect2d &src, const Rect2d &dst);
};

#endif /* GUI_HPP_ */
