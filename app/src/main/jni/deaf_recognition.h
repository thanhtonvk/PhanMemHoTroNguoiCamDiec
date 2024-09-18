//
// Created by TonSociu on 16/9/24.
//

#ifndef NGUOICAM_DEAF_RECOGNITION_H
#define NGUOICAM_DEAF_RECOGNITION_H
#include <opencv2/core/core.hpp>
#include <net.h>
#include "yolo.h"
class DeafRecognition {
public:
    DeafRecognition();

    int load(AAssetManager *mgr);

    int predict(const cv::Mat& src, Object &object, std::vector<float> &result);

    static int draw(cv::Mat &rgb, Object &object, std::vector<float> &result);

private:
    ncnn::Net model;
};


#endif //NGUOICAM_DEAF_RECOGNITION_H
