#include "Gui.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/tracking.hpp>

Gui::Gui(const char *window_name, Point initial_position) {
    this->window_name = window_name;
    this->initial_position = initial_position;
}

Gui::~Gui() {
}

void Gui::open(const Mat &frame) {
    frame.copyTo(current_frame);
    imshow(window_name, current_frame);
    moveWindow(window_name, initial_position.x, initial_position.y);
}

void Gui::update(const Mat &frame, const std::vector<Rect2d> &mask_rect, bool tracking, int frame_id, bool debug) {
    current_frame_orig = frame;
    frame.copyTo(current_frame);

    if (tracking) {
        // write overlay image.
        for (int i = 0; i < mask_rect.size(); i++) {
            if (i >= image_mask_list.size()) {
                rectangle(current_frame, mask_rect[i], Scalar(0, 0, 255), 2);
            }
            else {
                const double weight = 0.3;
                double ratio = sqrt((mask_rect[i].width * mask_rect[i].height) / (image_mask_list[i].cols * image_mask_list[i].rows));
                ratio = weight + (ratio * (1.0 - weight));
                double dst_w = image_mask_list[i].cols * ratio;
                double dst_h = image_mask_list[i].rows * ratio;
                double dst_x = mask_rect[i].x + (mask_rect[i].width - dst_w) / 2;
                double dst_y = mask_rect[i].y + (mask_rect[i].height - dst_h) / 2;

                Rect2d src = Rect2d(0, 0, image_mask_list[i].cols, image_mask_list[i].rows);
                Rect2d dst = Rect2d(dst_x, dst_y, dst_w, dst_h);

                Mat warp_mat;
                getLinearTransformMatrix(warp_mat, src, dst);
                drawImage(image_mask_list[i], current_frame, warp_mat);
            }
        }
    }

    if (debug) {
        // write frame id.
        Scalar color = Scalar(0, 255, 0);
        char text[256];
        sprintf(text, "#%05d", frame_id);
        putText(current_frame, text, Point(5, 30), FONT_HERSHEY_SIMPLEX, 0.8, color, 2, CV_AA);

        // write debug info.
        color = Scalar(0, 255, 0);
        int font = FONT_HERSHEY_SIMPLEX;
        int dy = 16;
        int thickness = 1;
        double scale = 0.4;

        Point offset(5, 50);

        sprintf(text, "tracking: %s", tracking ? "T" : "F");
        putText(current_frame, text, offset, font, scale, color, thickness, CV_AA);
        offset.y += dy;
        for (int i = 0; i < mask_rect.size(); i++) {
            sprintf(text, "[%d] (%03d,%03d)", i, (int)mask_rect[i].x, (int)mask_rect[i].y);
            putText(current_frame, text, offset, font, scale, color, thickness, CV_AA);
            offset.y += dy;
        }
    }

    imshow(window_name, current_frame);
}

void Gui::close() {
    destroyWindow(window_name);
}

void Gui::selectRoi(std::vector<Rect2d> &roi) {
    Mat image_select_roi;
    current_frame_orig.copyTo(image_select_roi);
    putText(image_select_roi, "SELECT ROI", Point(5, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2, CV_AA);

    selectROI(window_name, image_select_roi, roi, false);
}

void Gui::addMaskImage(const Mat &image) {
    Mat image_mask;
    image.copyTo(image_mask);
    image_mask_list.push_back(image_mask);
}

// TODO: overwrite background image considering alpha channel of foreground image.
//       any better solution?
void Gui::drawImage(const Mat &fore_image, Mat &back_image, const Mat &affine) {
    std::vector<Mat> rgba, rgb, alpha, inv_alpha;
    Mat fore_rgb, fore_alpha, fore_inv_alpha;

    Mat tmp(back_image.rows, back_image.cols, fore_image.type());
    tmp = Scalar::all(0);
    warpAffine(fore_image, tmp, affine, tmp.size(), CV_INTER_CUBIC, BORDER_TRANSPARENT);
    split(tmp, rgba);

    rgb.push_back(rgba[0]);
    rgb.push_back(rgba[1]);
    rgb.push_back(rgba[2]);
    merge(rgb, fore_rgb);

    alpha.push_back(rgba[3]);
    alpha.push_back(rgba[3]);
    alpha.push_back(rgba[3]);
    merge(alpha, fore_alpha);

    inv_alpha.push_back(0xff - rgba[3]);
    inv_alpha.push_back(0xff - rgba[3]);
    inv_alpha.push_back(0xff - rgba[3]);
    merge(inv_alpha, fore_inv_alpha);

    back_image = fore_rgb.mul(fore_alpha, 1.0/0xff) + back_image.mul(fore_inv_alpha, 1.0/0xff);
}

void Gui::getLinearTransformMatrix(Mat &mat, const Rect2d &src, const Rect2d &dst) {
    Point2f srcTri[3], dstTri[3];

    srcTri[0] = Point2f(src.x,             src.y             );
    srcTri[1] = Point2f(src.x + src.width, src.y             );
    srcTri[2] = Point2f(src.x,             src.y + src.height);

    dstTri[0] = Point2f(dst.x,             dst.y             );
    dstTri[1] = Point2f(dst.x + dst.width, dst.y             );
    dstTri[2] = Point2f(dst.x,             dst.y + dst.height);

    mat = getAffineTransform(srcTri, dstTri);
}
