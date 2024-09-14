package com.tondz.nguoicam;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.view.Surface;

import java.util.List;

public class NguoiMuSDK {
    public native boolean loadModel(AssetManager assetManager);

    public native boolean openCamera(int facing);

    public native boolean closeCamera();

    public native boolean setOutputWindow(Surface surface);

    public native List<String> getListResult();

    public native String getEmotion();

    static {
        System.loadLibrary("nguoimusdk");
    }
}
