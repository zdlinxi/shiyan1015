#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// 输入一根灯条的轮廓，return{上端点, 下端点}
std::pair<cv::Point2f, cv::Point2f> barEndpoints(const std::vector<cv::Point>& contour) {
    cv::RotatedRect rr = cv::minAreaRect(contour);

    cv::Point2f p[4];
    rr.points(p);                 // 直接用成员函数

    int a = 0, b = 1;
    double best = 1e18;
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        double d = cv::norm(p[i] - p[j]);
        if (d < best) { best = d; a = i; b = j; }
    }

    cv::Point2f e1 = (p[a] + p[b]) * 0.5f;
    cv::Point2f e2 = (p[(a + 2) % 4] + p[(b + 2) % 4]) * 0.5f;

    if (e1.y > e2.y) std::swap(e1, e2);
    return {e1, e2};
}

// 伽马增强
cv::Mat enhance(const cv::Mat& src) {
    static uchar lut[256];
    static bool ready = false;
    if (!ready) {
        for (int i = 0; i < 256; ++i)
            lut[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, 0.45) * 255.0);
        ready = true;
    }

    cv::Mat dst = src.clone();
    int total = dst.rows * (int)dst.step;
    uchar* data = dst.data;
    for (int i = 0; i < total; ++i)
        data[i] = lut[data[i]];
    return dst;
}


//main函数部分
int main() {
    cv::VideoCapture cap("assets/zhaungjiaban.avi");
    if (!cap.isOpened()) {
        std::cerr << "无法打开视频" << std::endl;
        return -1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 1.0 || fps > 240.0) fps = 25.0;
    int w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    cv::VideoWriter writer("assets/result.avi",
                           cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                           fps, cv::Size(w, h));
    if (!writer.isOpened()) {
        std::cerr << "VideoWriter 打开失败" << std::endl;
        return -1;
    }

    const cv::Scalar GREEN(0, 255, 0);
    int zhenlv = 0;

    cv::Mat frame;
    while (cap.read(frame)) {
        //检测原图
        cv::Mat hsv, mask;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, cv::Scalar(0, 100, 60), cv::Scalar(20, 255, 255), mask);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::vector<cv::Point>> bars;
        for (const auto& c : contours) {
            double area = cv::contourArea(c);
            cv::Rect r = cv::boundingRect(c);
            if (r.width == 0) continue;
            double aspect = (double)r.height / r.width;
            if (area < 20.0 || r.height < 8 || aspect < 2.0) continue;

            cv::Mat cmask = cv::Mat::zeros(mask.size(), CV_8UC1);
            std::vector<std::vector<cv::Point>> one{c};
            cv::drawContours(cmask, one, 0, cv::Scalar(255), -1);
            if (cv::mean(hsv, cmask)[2] < 115.0) continue;   // 倒影处理

            bars.push_back(c);
        }

        //标绿+连线（增强图）
        cv::Mat vis = enhance(frame);

        std::vector<std::pair<cv::Point2f, cv::Point2f>> ends;
        for (const auto& c : bars) {
            cv::RotatedRect rr = cv::minAreaRect(c);

            cv::Point2f p[4];
            rr.points(p);
            for (int i = 0; i < 4; ++i)
                cv::line(vis, p[i], p[(i + 1) % 4], GREEN, 1);

            ends.push_back(barEndpoints(c));
        }

        if (ends.size() == 2) {
            if (ends[0].first.x > ends[1].first.x) std::swap(ends[0], ends[1]);
            cv::line(vis, ends[0].first, ends[1].first, GREEN, 1);
            cv::line(vis, ends[0].second, ends[1].second, GREEN, 1);
        }

        writer.write(vis);
        zhenlv++;
    }

    writer.release();
    cap.release();
    std::cout << "输出: assets/result.avi  共 " << zhenlv << " 帧  "
              << w << "x" << h << "   " << fps << "fps" << std::endl;
    return 0;
}