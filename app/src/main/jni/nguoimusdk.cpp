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


#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "emotion_recognition.h"
#include "deaf_recognition.h"

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

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat &rgb) const {
    {
        ncnn::MutexLockGuard g(lock);
        if (g_yolo && g_scrfd && g_deaf) {
            g_yolo->detect(rgb, objects);
            g_scrfd->detect(rgb, faceObjects);
            scoreEmotions.clear();
            scoreDeafs.clear();
            for (auto &faceObject: faceObjects) {
                g_emotion->predict(rgb, faceObject, scoreEmotions);
            }
            for (auto &object: objects) {
                g_deaf->predict(rgb, object, scoreDeafs);
            }
            for (auto &faceObject: faceObjects) {
                g_emotion->draw(rgb, faceObject, scoreEmotions);
            }
            for (auto &object: objects) {
                if (object.label == 0) {
                    g_deaf->draw(rgb, object, scoreDeafs);
                }

            }
            g_yolo->draw(rgb, objects);
        }
    }
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
Java_com_tondz_nguoicam_NguoiMuSDK_loadModel(JNIEnv *env, jobject thiz, jobject assetManager) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    ncnn::MutexLockGuard g(lock);
    const char *modeltype = "s";
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
Java_com_tondz_nguoicam_NguoiMuSDK_openCamera(JNIEnv *env, jobject thiz, jint facing) {
    if (facing < 0 || facing > 1)
        return JNI_FALSE;
    g_camera->open((int) facing);

    return JNI_TRUE;
}

extern "C" jboolean
Java_com_tondz_nguoicam_NguoiMuSDK_closeCamera(JNIEnv *env, jobject thiz) {
    g_camera->close();

    return JNI_TRUE;
}

extern "C" jboolean
Java_com_tondz_nguoicam_NguoiMuSDK_setOutputWindow(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
    g_camera->set_window(win);
    return JNI_TRUE;
}

}
extern "C" jobject
Java_com_tondz_nguoicam_NguoiMuSDK_getListResult(JNIEnv *env, jobject thiz) {
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
    jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);

    // Get the add method of ArrayList
    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
    for (const Object &obj: objects) {
        if (obj.label == 0) {
            std::ostringstream oss;
            oss << obj.label;
            std::string objName = oss.str();
            jstring javaString = env->NewStringUTF(objName.c_str());  // Convert to jstring
            env->CallBooleanMethod(arrayList, arrayListAdd, javaString);
            env->DeleteLocalRef(javaString);  // Clean up local reference
        }
    }
    objects.clear();
    return arrayList;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_tondz_nguoicam_NguoiMuSDK_getEmotion(JNIEnv *env, jobject thiz) {
    if (!scoreEmotions.empty()) {
        std::ostringstream oss;

        // Convert each element to string and add it to the stream
        for (size_t i = 0; i < scoreEmotions.size(); ++i) {
            if (i != 0) {
                oss << ",";  // Add a separator between elements
            }
            oss << scoreEmotions[i];
        }

        // Convert the stream to a string
        std::string embeddingStr = oss.str();
        scoreEmotions.clear();
        return env->NewStringUTF(embeddingStr.c_str());
    }
    return env->NewStringUTF("");
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_tondz_nguoicam_NguoiMuSDK_getDeaf(JNIEnv *env, jobject thiz) {
    if (!scoreDeafs.empty()) {
        std::ostringstream oss;

        // Convert each element to string and add it to the stream
        for (size_t i = 0; i < scoreDeafs.size(); ++i) {
            if (i != 0) {
                oss << ",";  // Add a separator between elements
            }
            oss << scoreDeafs[i];
        }

        // Convert the stream to a string
        std::string embeddingStr = oss.str();
        scoreDeafs.clear();
        return env->NewStringUTF(embeddingStr.c_str());
    }
    return env->NewStringUTF("");
}