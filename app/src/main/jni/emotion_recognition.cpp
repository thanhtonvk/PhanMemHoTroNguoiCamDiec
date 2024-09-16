//
// Created by thanh on 9/14/2024.
//

#include "emotion_recognition.h"
#include <string.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cpu.h"
#include <opencv2/opencv.hpp>



cv::Rect resizeImage(FaceObject faceobject) {
    // Get the original coordinates
    int left = faceobject.rect.x;
    int top = faceobject.rect.y;
    int w = faceobject.rect.width;
    int h = faceobject.rect.height;
    int maxSize = std::max(w, h);

    // Tính lại tọa độ sao cho hình vuông nằm giữa hình chữ nhật
    int newLeft = left - (maxSize - w) / 2;
    int newTop = top - (maxSize - h) / 2;

    // Đảm bảo tọa độ không vượt quá biên của hình ảnh
    newLeft = std::max(0, newLeft);
    newTop = std::max(0, newTop);


    return cv::Rect(newLeft, newTop, maxSize, maxSize);
}

EmotionRecognition::EmotionRecognition() {

}

int EmotionRecognition::load(AAssetManager *mgr) {
    model.clear();
    ncnn::set_cpu_powersave(2);
    ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());
    model.opt = ncnn::Option();
    model.opt.num_threads = ncnn::get_big_cpu_count();
    char parampath[256];
    char modelpath[256];
    sprintf(parampath, "model.param");
    sprintf(modelpath, "model.bin");
    model.load_param(mgr, parampath);
    model.load_model(mgr, modelpath);

    return 0;
}

int EmotionRecognition::predict(cv::Mat src, FaceObject &faceobject,
                                std::vector<float> &result) {
    cv::Rect newRect = resizeImage(faceobject);
    if (newRect.x >= 0 && newRect.y >= 0 &&
        newRect.width > 0 && newRect.height > 0 &&
        newRect.x + newRect.width <= src.cols &&
        newRect.y + newRect.height <= src.rows) {
        cv::Mat croppedImage = src(newRect);
        ncnn::Mat in_net = ncnn::Mat::from_pixels_resize(croppedImage.clone().data,
                                                         ncnn::Mat::PIXEL_RGB, croppedImage.cols,
                                                         croppedImage.rows,
                                                         128, 128);
        float norm[3] = {1 / 127.5f, 1 / 127.5f, 1 / 127.5f};
        float mean[3] = {127.5f, 127.5f, 127.5f};
        in_net.substract_mean_normalize(mean, norm);
        ncnn::Extractor extractor = model.create_extractor();
        extractor.input("in0", in_net);
        ncnn::Mat outBlob;
        extractor.extract("out0", outBlob);
        for (int i = 0; i < outBlob.w; i++) {
            float test = outBlob.row(0)[i];
            result.push_back(test);
        }
    }

    return 0;
}

int EmotionRecognition::draw(cv::Mat &rgb, FaceObject &faceobject, std::vector<float> &result) {
    static const char *class_names[] = {
            "tuc gian", "ghe tom", "so hai", "vui ve",
            "buon", "bat ngo", "tu nhien", "khinh miet"
    };
    int index = 0;
    float scoreMax = 0;
    for (int i = 0; i < result.size(); i++) {
        if (scoreMax < result[i]) {
            index = i;
            scoreMax = result[i];
        }
    }
    static const unsigned char colors[19][3] = {
            {54,  67,  244},
            {99,  30,  233},
            {176, 39,  156},
            {183, 58,  103},
            {181, 81,  63},
            {243, 150, 33},
            {244, 169, 3},
            {212, 188, 0},
            {136, 150, 0},
            {80,  175, 76},
            {74,  195, 139},
            {57,  220, 205},
            {59,  235, 255},
            {7,   193, 255},
            {0,   152, 255},
            {34,  87,  255},
            {72,  85,  121},
            {158, 158, 158},
            {139, 125, 96}
    };

    int color_index = 0;


    const FaceObject &obj = faceobject;
    cv::Rect newRect = resizeImage(obj);


    const unsigned char *color = colors[color_index % 19];
    color_index++;

    cv::Scalar cc(color[0], color[1], color[2]);

    cv::rectangle(rgb, newRect, cc, 2);

    char text[256];
    sprintf(text, "%s %.1f%%", class_names[index], result[index] * 100);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int x = newRect.x;
    int y = newRect.y - label_size.height - baseLine;
    if (y < 0)
        y = 0;
    if (x + label_size.width > rgb.cols)
        x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)), cc,
                  -1);

    cv::Scalar textcc = (color[0] + color[1] + color[2] >= 381) ? cv::Scalar(0, 0, 0)
                                                                : cv::Scalar(255, 255, 255);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                textcc, 1);


    return 0;
}
