project(nguoimusdk)

cmake_minimum_required(VERSION 3.10)

set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/opencv-mobile-4.6.0-android/sdk/native/jni)
find_package(OpenCV REQUIRED core imgproc highgui)

set(ncnn_DIR ${CMAKE_SOURCE_DIR}/ncnn-20230517-android-vulkan/${ANDROID_ABI}/lib/cmake/ncnn)
find_package(ncnn REQUIRED)

add_library(nguoimusdk SHARED nguoimusdk.cpp yolo.cpp ndkcamera.cpp scrfd.cpp emotion_recognition.cpp)

target_link_libraries(nguoimusdk ncnn ${OpenCV_LIBS} camera2ndk mediandk)


