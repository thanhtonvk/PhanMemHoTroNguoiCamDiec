package com.tondz.phanmemhotrocamdiec;

import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Handler;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;


import com.tondz.phanmemhotrocamdiec.databinding.ActivityMainBinding;

import java.util.List;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final int REQUEST_CAMERA = 1345;
    ActivityMainBinding binding;
    NguoiMuSDK nguoiMuSDK = new NguoiMuSDK();
    private int facing = 0;
    TextToSpeech textToSpeech;
    private boolean canPlaySound = true;
    Handler handler;
    Runnable runnable;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());


        init();
        onClick();
        reload();
        handler = new Handler();
        runnable = new Runnable() {
            @Override
            public void run() {
                canPlaySound = true;
            }
        };
        getObject();
    }

    private void getObject() {
        new Thread(() -> {
            while (true) {
                String deafScore = nguoiMuSDK.getDeaf();
                String emotionScore = nguoiMuSDK.getEmotion();
                if (!deafScore.isEmpty() && !emotionScore.isEmpty()) {
                    String emotion = getEmotion(emotionScore);
                    String deaf = getDeaf(deafScore);
                    String speak = "";
                    if (emotion.equalsIgnoreCase("sợ") && deaf.equalsIgnoreCase("sợ")) {
                        speak = "sợ";
                    }
                    if (emotion.equalsIgnoreCase("vui vẻ") && deaf.equalsIgnoreCase("rất vui được gặp bạn")) {
                        speak = "rất vui được gặp bạn";
                    }
                    if (emotion.equalsIgnoreCase("tức giận") && deaf.equalsIgnoreCase("không thích")) {
                        speak = "không thích";
                    }
                    if (emotion.equalsIgnoreCase("vui vẻ") && deaf.equalsIgnoreCase("thích")) {
                        speak = "thích";
                    }
                    if (emotion.equalsIgnoreCase("tự nhiên") && deaf.equalsIgnoreCase("xin chào")) {
                        speak = "xin chào";
                    }
                    if (canPlaySound) {
                        if (!speak.isEmpty()) {
                            speakVoice(speak);
                            canPlaySound = false;
                            handler.postDelayed(runnable, 3000);
                        }

                    }

                }
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }
        }).start();
    }

    private void onClick() {
        binding.btnFlip.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int new_facing = 1 - facing;
                nguoiMuSDK.closeCamera();
                nguoiMuSDK.openCamera(new_facing);
                facing = new_facing;
            }
        });

    }

    private String getDeaf(String scoreDeaf) {
        String[] arr = scoreDeaf.split(",");
        float maxScore = 0;
        int maxIdx = 0;
        for (int i = 0; i < arr.length; i++) {
            float score = Float.parseFloat(arr[i]);
            if (score > maxScore) {
                maxScore = score;
                maxIdx = i;
            }
        }
        return Common.classNames[maxIdx];
    }

    private String getEmotion(String scoreEmotion) {
        String[] arr = scoreEmotion.split(",");
        float maxScore = 0;
        int maxIdx = 0;
        for (int i = 0; i < arr.length; i++) {
            float score = Float.parseFloat(arr[i]);
            if (score > maxScore) {
                maxScore = score;
                maxIdx = i;
            }
        }
        return Common.emotionClasses[maxIdx];
    }

    private void init() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        binding.cameraview.getHolder().setFormat(PixelFormat.RGBA_8888);
        binding.cameraview.getHolder().addCallback(this);
        textToSpeech = new TextToSpeech(getApplicationContext(), new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int i) {
                if (i != TextToSpeech.ERROR) {
                    textToSpeech.setLanguage(Locale.forLanguageTag("vi-VN"));
                }
            }
        });
    }

    private void reload() {
        boolean ret_init = nguoiMuSDK.loadModel(getAssets());
        if (!ret_init) {
            Log.e("NhanDienNguoiThanActivity", "yolov8ncnn loadModel failed");
        } else {
            Log.e("NhanDienNguoiThanActivity", "yolov8ncnn loadModel ok");
        }
    }

    private void speakVoice(String noiDung) {
        textToSpeech.speak(noiDung, TextToSpeech.QUEUE_FLUSH, null);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        nguoiMuSDK.setOutputWindow(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        nguoiMuSDK.closeCamera();
    }

    @Override
    protected void onResume() {
        super.onResume();
        nguoiMuSDK.openCamera(facing);
    }

    @Override
    protected void onPause() {
        super.onPause();
        nguoiMuSDK.closeCamera();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        nguoiMuSDK.closeCamera();
    }
}