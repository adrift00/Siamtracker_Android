package com.example.siamtracker;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private static final int REQUEST_EXTERNAL_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {
            "android.permission.READ_EXTERNAL_STORAGE",
            "android.permission.WRITE_EXTERNAL_STORAGE"};

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("siamtracker");
    }

    public static void verifyStoragePermissions(Activity activity) {
        try {
            //检测是否有写的权限
            int permission = ActivityCompat.checkSelfPermission(activity,
                    "android.permission.WRITE_EXTERNAL_STORAGE");
            if (permission != PackageManager.PERMISSION_GRANTED) {
                // 没有写的权限，去申请写的权限，会弹出对话框
                ActivityCompat.requestPermissions(activity, PERMISSIONS_STORAGE, REQUEST_EXTERNAL_STORAGE);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        verifyStoragePermissions(this);
        //copy to sd scard
        try {
            copyBigDataToSD("siamrpn_examplar.mnn");
            copyBigDataToSD("siamrpn_search.mnn");
            copyBigDataToSD("00000001.jpg");
            copyBigDataToSD("00000002.jpg");
        } catch (IOException e) {
            e.printStackTrace();
        }
        // model init
        File sdDir = Environment.getExternalStorageDirectory();//获取跟目录
        String sdPath = sdDir.toString() + "/siamtracker/";
        siamtrackerInitModel(sdPath);
        String examplarPath = sdPath + "00000001.jpg";
        float[] bbox = {365.2075f, 194.595f, 106.1308f, 96.4144f};
        siamtrackerInit(examplarPath, bbox);
        String searchPath = sdPath + "00000002.jpg";
        siamtrackerTrack(searchPath);


    }

    private void copyBigDataToSD(String strOutFileName) throws IOException {
        Log.i(TAG, "start copy file " + strOutFileName);
        File sdDir = Environment.getExternalStorageDirectory();//get root dir
        File file = new File(sdDir.toString() + "/siamtracker/");
        boolean haha = false;
        if (!file.exists()) {
            haha = file.mkdirs();
        }
        String tmpFile = sdDir.toString() + "/siamtracker/" + strOutFileName;
        File f = new File(tmpFile);
        if (f.exists()) {
            Log.i(TAG, "file exists " + strOutFileName);
            return;
        }
        InputStream myInput;
        OutputStream myOutput = new FileOutputStream(sdDir.toString() + "/siamtracker/" + strOutFileName);
        myInput = this.getAssets().open(strOutFileName);
        byte[] buffer = new byte[1024];
        int length = myInput.read(buffer);
        while (length > 0) {
            myOutput.write(buffer, 0, length);
            length = myInput.read(buffer);
        }
        myOutput.flush();
        myInput.close();
        myOutput.close();
        Log.i(TAG, "end copy file " + strOutFileName);

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void siamtrackerInitModel(String path);

    public native void siamtrackerInit(String img_path, float[] bbox);

    public native void siamtrackerTrack(String img_path);
}
