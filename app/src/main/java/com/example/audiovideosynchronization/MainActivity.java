package com.example.audiovideosynchronization;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    private static final String tag = "MainActivity.class";
    FFmpegSurface fmpegSurface;
    SurfaceView surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        surface = findViewById(R.id.myview);
        Button btn = findViewById(R.id.startBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                fmpegSurface.start();
            }
        });
        fmpegSurface = new FFmpegSurface(this, surface);
        File file = new File(Environment.getExternalStorageDirectory(), "ab123.mp4");
        if (file.exists()) {
            Log.e(tag, "---------存在文件>>>>>>:" );
            fmpegSurface.prepare(file.getAbsolutePath());
            fmpegSurface.setOnErrorListener(new FFmpegSurface.FFmpegOnErrorListener() {
                @Override
                public void onError(int errorCode) {
                    Log.e(tag, "---------onError>>>>>>:" + errorCode);
                }
            });
            fmpegSurface.setOnPreparedListener(new FFmpegSurface.FFmpegOnPreparedListener() {
                @Override
                public void onPrepared() {
                    Log.e(tag, "---------onPrepared>>>>>>:");

                }
            });
            fmpegSurface.setOnProgressListener(new FFmpegSurface.FFmpegOnProgressListener() {
                @Override
                public void onProgress(int progress) {
                    Log.e(tag, "---------onProgress>>>>>>:" + progress);
                }
            });
        }else{
            Log.e(tag, "---------文件不存在");
        }
    }

}
