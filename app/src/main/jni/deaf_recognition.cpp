#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cpu.h"
#include <opencv2/opencv.hpp>
#include "deaf_recognition.h"

DeafRecognition::DeafRecognition() {}

int DeafRecognition::load(AAssetManager *mgr) {
    model.clear();
    ncnn::set_cpu_powersave(2);
    ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());
    model.opt = ncnn::Option();
    model.opt.num_threads = ncnn::get_big_cpu_count();
    char parampath[256];
    char modelpath[256];
    sprintf(parampath, "cam_diec.param");
    sprintf(modelpath, "cam_diec.bin");
    model.load_param(mgr, parampath);
    model.load_model(mgr, modelpath);

    return 0;
}

int DeafRecognition::predict(const cv::Mat &src, Object &object, std::vector<float> &result) {
    if (object.rect.x >= 0 && object.rect.y >= 0 &&
        object.rect.width > 0 && object.rect.height > 0 &&
        object.rect.x + object.rect.width <= src.cols &&
        object.rect.y + object.rect.height <= src.rows) {
        cv::Mat croppedImage = src(object.rect);
        ncnn::Mat in_net = ncnn::Mat::from_pixels_resize(croppedImage.clone().data,
                                                         ncnn::Mat::PIXEL_RGB,
                                                         croppedImage.cols, croppedImage.rows, 128,
                                                         128);
        float norm[3] = {1 / 127.5f, 1 / 127.5f, 1 / 127.5f};
        float mean[3] = {127.5f, 127.5f, 127.5f};
        in_net.substract_mean_normalize(mean, norm);
        ncnn::Extractor extractor = model.create_extractor();
        extractor.input("in0", in_net);
        ncnn::Mat outBlob;
        extractor.extract("out0", outBlob);
        result.reserve(outBlob.w);
        for (int i = 0; i < outBlob.w; i++) {
            result.push_back(outBlob[i]);
        }
    }
    return 0;
}

int DeafRecognition::draw(cv::Mat &rgb, Object &object, std::vector<float> &result) {
    static const char *class_names[] = {
            "cam on", "hen gap lai", "khoe", "khong thich", "rat vui duoc gap ban", "so",
            "tam biet", "thich", "xin chao", "xin loi"
    };
    static const unsigned char colors[1][3] = {
            {54, 67, 244}
    };

    if (!result.empty()) {
        int index = 0;
        float scoreMax = 0;
        for (int i = 0; i < result.size(); i++) {
            if (scoreMax < result[i]) {
                scoreMax = result[i];
                index = i;
            }
        }
        if (scoreMax >= 0.9) {
            const unsigned char *color = colors[0];
            cv::Scalar cc(color[0], color[1], color[2]);

            cv::rectangle(rgb, object.rect, cc, 2);

            std::string text = cv::format("%s %.1f%%", class_names[index], scoreMax * 100);

            int baseLine = 0;
            cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1,
                                                  &baseLine);

            int x = object.rect.x;
            int y = object.rect.y - label_size.height - baseLine;
            y = std::max(y, 0);
            x = std::min(x, rgb.cols - label_size.width);

            cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                        cv::Size(label_size.width, label_size.height + baseLine)),
                          cc, -1);

            cv::Scalar textcc = (color[0] + color[1] + color[2] >= 381) ? cv::Scalar(0, 0, 0)
                                                                        : cv::Scalar(255, 255, 255);

            cv::putText(rgb, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX,
                        0.5, textcc, 1);
        }
    }
    return 0;
}
