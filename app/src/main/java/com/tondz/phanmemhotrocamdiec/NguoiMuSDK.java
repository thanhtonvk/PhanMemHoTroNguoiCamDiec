package com.tondz.phanmemhotrocamdiec;

import android.content.res.AssetManager;
import android.view.Surface;

import java.util.List;

public class NguoiMuSDK {
    public native boolean loadModel(AssetManager assetManager);

    public native boolean openCamera(int facing);

    public native boolean closeCamera();

    public native boolean setOutputWindow(Surface surface);

    public native List<String> getListResult();

    public native String getEmotion();
    public native String getDeaf();
    static {
        System.loadLibrary("nguoimusdk");
    }
}
