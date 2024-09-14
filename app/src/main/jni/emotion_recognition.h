//
// Created by thanh on 9/14/2024.
//

#ifndef NGUOICAM_EMOTION_RECOGNITION_H
#define NGUOICAM_EMOTION_RECOGNITION_H

#include <opencv2/core/core.hpp>
#include <net.h>
#include "scrfd.h"

class EmotionRecognition {
public:
    EmotionRecognition();

    int load(AAssetManager *mgr);

    int predict(cv::Mat src, FaceObject &faceobject, std::vector<float> &result);

    int draw(cv::Mat &rgb, FaceObject &faceobject, std::vector<float> &result);

private:
    ncnn::Net model;
};


#endif //NGUOICAM_EMOTION_RECOGNITION_H
