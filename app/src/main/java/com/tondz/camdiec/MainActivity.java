package com.tondz.camdiec;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.tondz.camdiec.databinding.ActivityMainBinding;

import java.io.IOException;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final int REQUEST_CAMERA = 1345;
    ActivityMainBinding binding;
    NguoiMuSDK nguoiMuSDK = new NguoiMuSDK();
    private int facing = 0;
    private boolean canPlaySound = true;
    Handler handler;
    Runnable runnable;
    MediaPlayer mediaPlayer;

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
                String emotionResult = nguoiMuSDK.getEmotion();
                if (!emotionResult.isEmpty()) {
                    String emotionScore = emotionResult.split(";")[0];
                    String deafScore = emotionResult.split(";")[1];
                    String emotion = getEmotion(emotionScore);
                    String deaf = getDeaf(deafScore);
                    int source = getSource(emotion, deaf);
                    if (source != 0 && canPlaySound) {
                        Log.e("TAG", "getObject: " + emotion + " " + deaf);
                        mediaPlayer = MediaPlayer.create(this, source);
                        mediaPlayer.start();
                        canPlaySound = false;
                        handler.postDelayed(runnable, 2000);
                    }
                }
                try {
                    Thread.sleep(30);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }
        }).start();
    }

    private static int getSource(String emotion, String deaf) {
        Log.e("TAG", "getSource: " + emotion + deaf);
        int source = 0;
        if (emotion.equalsIgnoreCase("so") && deaf.equalsIgnoreCase("so")) {
            source = R.raw.so;
        } else if (emotion.equalsIgnoreCase("vui ve") && deaf.equalsIgnoreCase("rat vui duoc gap ban")) {
            source = R.raw.rat_vui_duoc_gap_ban;
        } else if (emotion.equalsIgnoreCase("tuc gian") && deaf.equalsIgnoreCase("khong thich")) {
            source = R.raw.khong_thich;
        } else if (emotion.equalsIgnoreCase("vui ve") && deaf.equalsIgnoreCase("cam on")) {
            source = R.raw.cam_on;
        } else if (emotion.equalsIgnoreCase("vui ve") && deaf.equalsIgnoreCase("khoe")) {
            source = R.raw.khoe;
        } else if (emotion.equalsIgnoreCase("vui ve") && deaf.equalsIgnoreCase("thich")) {
            source = R.raw.thich;
        } else if (emotion.equalsIgnoreCase("buon") && deaf.equalsIgnoreCase("xin loi")) {
            source = R.raw.xin_loi;
        } else if (emotion.equalsIgnoreCase("tu nhien") && deaf.equalsIgnoreCase("hen gap lai")) {
            source = R.raw.hen_gap_lai;
        } else if (emotion.equalsIgnoreCase("tu nhien") && deaf.equalsIgnoreCase("xin chao")) {
            source = R.raw.xin_chao;
        }
//        else if (emotion.equalsIgnoreCase("buon") && deaf.equalsIgnoreCase("tam biet")) {
//            source = R.raw.tam_biet;
//        }
        return source;
    }

    private void onClick() {
        binding.btnChangeCamera.setOnClickListener(new View.OnClickListener() {
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
    }

    private void reload() {
        boolean ret_init = nguoiMuSDK.loadModel(getAssets());
        if (!ret_init) {
            Log.e("NhanDienNguoiThanActivity", "yolov8ncnn loadModel failed");
        } else {
            Log.e("NhanDienNguoiThanActivity", "yolov8ncnn loadModel ok");
        }
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
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (ContextCompat.checkSelfPermission(getApplicationContext(), android.Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, REQUEST_CAMERA);
        }
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
        if (mediaPlayer != null) {
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }
}