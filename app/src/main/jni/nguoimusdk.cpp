// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>

#include "yolo.h"

#include "ndkcamera.h"
#include "scrfd.h"
#include "emotion_recognition.h"
#include "deaf_recognition.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

static Yolo *g_yolo = 0;
static SCRFD *g_scrfd = 0;
static EmotionRecognition *g_emotion = 0;
static DeafRecognition *g_deaf = 0;
static ncnn::Mutex lock;


static std::vector<Object> objects;
static std::vector<FaceObject> faceObjects;
static std::vector<float> scoreEmotions;
static std::vector<float> scoreDeafs;

static int draw_fps(cv::Mat &rgb) {
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f) {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--) {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f) {
            return 0;
        }

        for (int i = 0; i < 10; i++) {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat &rgb) const {
    {
        ncnn::MutexLockGuard g(lock);
        if (g_yolo && g_deaf && g_scrfd && g_emotion) {

            g_yolo->detect(rgb, objects);
            g_scrfd->detect(rgb, faceObjects);
            if (!objects.empty() && !faceObjects.empty()) {
                scoreDeafs.clear();
                scoreEmotions.clear();
                g_deaf->predict(rgb, objects[0], scoreDeafs);
                g_emotion->predict(rgb, faceObjects[0], scoreEmotions);
                g_emotion->draw(rgb, faceObjects[0], scoreEmotions);
                g_deaf->draw(rgb, objects[0], scoreDeafs);
            }
        }
    }
    draw_fps(rgb);
}

static MyNdkCamera *g_camera = 0;

extern "C" {
JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_camera = new MyNdkCamera;

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    {
        ncnn::MutexLockGuard g(lock);
        delete g_yolo;
        g_yolo = 0;
        delete g_scrfd;
        g_scrfd = 0;

        delete g_emotion;
        g_emotion = 0;
        delete g_deaf;
        g_deaf = 0;
    }

    delete g_camera;
    g_camera = 0;
}


extern "C" jboolean
Java_com_tondz_camdiec_NguoiMuSDK_loadModel(JNIEnv *env, jobject thiz, jobject assetManager) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    ncnn::MutexLockGuard g(lock);
    const char *modeltype = "n";

    const float mean_vals[][3] =
            {
                    {103.53f, 116.28f, 123.675f},
                    {103.53f, 116.28f, 123.675f},
            };

    const float norm_vals[][3] =
            {
                    {1 / 255.f, 1 / 255.f, 1 / 255.f},
                    {1 / 255.f, 1 / 255.f, 1 / 255.f},
            };

    int target_size = 640;
    if (!g_yolo) {
        g_yolo = new Yolo;
    }
    g_yolo->load(mgr, modeltype, target_size, mean_vals[0],
                 norm_vals[0], false);

    if (!g_scrfd)
        g_scrfd = new SCRFD;
    g_scrfd->load(mgr, modeltype, false);

    if (!g_emotion)
        g_emotion = new EmotionRecognition;
    g_emotion->load(mgr);

    if (!g_deaf)
        g_deaf = new DeafRecognition;
    g_deaf->load(mgr);
    return JNI_TRUE;
}
extern "C" jboolean
Java_com_tondz_camdiec_NguoiMuSDK_openCamera(JNIEnv *env, jobject thiz, jint facing) {
    if (facing < 0 || facing > 1)
        return JNI_FALSE;
    g_camera->open((int) facing);

    return JNI_TRUE;
}

extern "C" jboolean
Java_com_tondz_camdiec_NguoiMuSDK_closeCamera(JNIEnv *env, jobject thiz) {
    g_camera->close();

    return JNI_TRUE;
}

extern "C" jboolean
Java_com_tondz_camdiec_NguoiMuSDK_setOutputWindow(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
    g_camera->set_window(win);
    return JNI_TRUE;
}

}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_tondz_camdiec_NguoiMuSDK_getEmotion(JNIEnv *env, jobject thiz) {

    if (!scoreEmotions.empty() && !scoreDeafs.empty()) {
        std::ostringstream oss;
        // Convert each element to string and add it to the stream
        for (size_t i = 0; i < scoreEmotions.size(); ++i) {
            if (i != 0) {
                oss << ",";  // Add a separator between elements
            }
            oss << scoreEmotions[i];
        }
        oss << ";";

        for (size_t i = 0; i < scoreDeafs.size(); ++i) {
            if (i != 0) {
                oss << ",";  // Add a separator between elements
            }
            oss << scoreDeafs[i];
        }


        std::string embeddingStr = oss.str();
        scoreEmotions.clear();
        scoreDeafs.clear();
        return env->NewStringUTF(embeddingStr.c_str());
    }
    return env->NewStringUTF("");
}



