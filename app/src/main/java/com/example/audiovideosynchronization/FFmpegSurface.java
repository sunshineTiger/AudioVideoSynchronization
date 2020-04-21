package com.example.audiovideosynchronization;

import android.content.Context;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FFmpegSurface implements SurfaceHolder.Callback {
    private static final String tag = "MainActivity.class";
    SurfaceHolder holder;
    FFmpegOnPreparedListener onPreparedListener;
    FFmpegOnProgressListener onProgressListener;
    FFmpegOnErrorListener onErrorListener;

    //    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public FFmpegSurface(Context xt, SurfaceView view) {

        if (null != this.holder) {
            this.holder.removeCallback(this);
        }
        this.holder = view.getHolder();
        this.holder.addCallback(this);
    }


    public void setOnPreparedListener(FFmpegOnPreparedListener onPreparedListener) {
        this.onPreparedListener = onPreparedListener;
    }

    public void setOnProgressListener(FFmpegOnProgressListener onProgressListener) {
        this.onProgressListener = onProgressListener;
    }

    public void setOnErrorListener(FFmpegOnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        native_set_surface(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        this.holder = holder;

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void prepare(String dataSource) {
        native_prepare(dataSource);
    }


    public void start() {
        native_start();
    }

    public void onPrepare() {
        Log.e(tag, "---------onPrepare>>>>>>:");
        if (null != onPreparedListener) {
            onPreparedListener.onPrepared();
        }
    }

    public void onProgress(int p) {
        Log.e(tag, "---------onProgress>>>>>>:");
        if (null != onProgressListener) {
            onProgressListener.onProgress(p);
        }
    }

    public void onError(int e) {
        Log.e(tag, "---------onError>>>>>>:");
        if (null != onErrorListener) {
            onErrorListener.onError(e);
        }
    }

    public native void native_set_surface(Surface surfaceView);

    public native void native_prepare(String path);

    public native void native_start();

    public interface FFmpegOnPreparedListener {
        public void onPrepared();
    }

    public interface FFmpegOnProgressListener {
        public void onProgress(int progress);
    }

    public interface FFmpegOnErrorListener {
        public void onError(int errorCode);

    }


}
