//
// Created by TonSociu on 13/8/24.
//

#ifndef NGUOIMU_SCRFD_H
#define NGUOIMU_SCRFD_H
#include <opencv2/core/core.hpp>

#include <net.h>

struct FaceObject
{
    cv::Rect_<float> rect;
    cv::Point2f landmark[5];
    float prob;
};

class SCRFD
{
public:
    int load(const char* modeltype, bool use_gpu = false);

    int load(AAssetManager* mgr, const char* modeltype, bool use_gpu = false);

    int detect(const cv::Mat& rgb, std::vector<FaceObject>& faceobjects, float prob_threshold = 0.5f, float nms_threshold = 0.45f);

    int draw(cv::Mat& rgb, const std::vector<FaceObject>& faceobjects);

private:
    ncnn::Net scrfd;
    bool has_kps;
};

#endif // SCRFD_H
