package com.example.siamtracker;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.MeteringRectangle;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.util.SparseIntArray;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("siamtracker");
    }

    private static final String TAG = "MainActivity";
    private static final int REQUEST_EXTERNAL_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {
            "android.permission.READ_EXTERNAL_STORAGE",
            "android.permission.WRITE_EXTERNAL_STORAGE"};
    private static final String TAG_PREVIEW = "预览";

    private static final SparseIntArray ORIENTATION = new SparseIntArray();

    static {
        ORIENTATION.append(Surface.ROTATION_0, 90);
        ORIENTATION.append(Surface.ROTATION_90, 0);
        ORIENTATION.append(Surface.ROTATION_180, 270);
        ORIENTATION.append(Surface.ROTATION_270, 180);
    }

    private String mCameraId;
    private CameraDevice mCameraDevice;
    private CameraCaptureSession mCaptureSession;

    private Size mPreviewSize;
    private CaptureRequest mPreviewRequest;
    private Surface mPreviewSurface;
    private CaptureRequest.Builder mPreviewRequestBuilder;

    private AutoFitTextureView mTextureView;

    private HandlerThread mBackgroundThread;     // 处理拍照等工作的子线程
    private Handler mBackgroundHandler;          // 上面定义的子线程的处理器
    private ImageReader mImageReader;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    // 初始化的矩形
    private Rect mInitRect = new Rect(0, 0, 0, 0);
    private Rect mTrackRect = new Rect(0, 0, 0, 0);
    private final int STROKE_WIDTH = 5;
    private Paint mPaint;  // 画框
    private Canvas mCanvas;     // 画布
    //
    private boolean mStartInit = false;
    private boolean mStartTrack = false;

    //surface 状态回调
    TextureView.SurfaceTextureListener mTextureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            //当SurefaceTexture可用的时候，设置相机参数并打开相机
            setupCamera(width, height);
            openCamera();
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            return false;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {

        }
    };
    // 摄像头状态回调
    private final CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(@NonNull CameraDevice camera) {
            mCameraDevice = camera;
            //开启预览
            startPreview();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice camera) {
            Log.i(TAG, "CameraDevice Disconnected");
        }

        @Override
        public void onError(@NonNull CameraDevice camera, int error) {
            Log.e(TAG, "CameraDevice Error");
        }
    };

    private View.OnTouchListener mOntouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            int x = (int) event.getX();
            int y = (int) event.getY();
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
//                    mInitRect.right += STROKE_WIDTH;
//                    mInitRect.bottom += STROKE_WIDTH;
                    mInitRect.left = x;
                    mInitRect.top = y;
                    mInitRect.right = mInitRect.left;
                    mInitRect.bottom = mInitRect.top;
                case MotionEvent.ACTION_MOVE:
//                    Rect old = new Rect(mInitRect.left,
//                            mInitRect.top,
//                            mInitRect.right + STROKE_WIDTH,
//                            mInitRect.bottom + STROKE_WIDTH);
                    mInitRect.right = x;
                    mInitRect.bottom = y;
//                    old.union(x, y);
                    break;
                case MotionEvent.ACTION_UP:
                    clearDraw();
                    transformBBox(mInitRect);
                    mCanvas = mSurfaceHolder.lockCanvas();   // 得到surfaceView的画布
                    mCanvas.drawRect(mInitRect, mPaint);
                    mSurfaceHolder.unlockCanvasAndPost(mCanvas);
                    mStartInit = true;
                    break;
                default:
                    break;
            }
            return true;//处理了触摸信息，消息不再传递
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        verifyStoragePermissions(this);
        //copy to sd scard
        try {
            copyBigDataToSD("siamrpn_mobi_pruning_examplar.mnn");
            copyBigDataToSD("siamrpn_mobi_pruning_search.mnn");
//            copyBigDataToSD("00000001.jpg");
//            copyBigDataToSD("00000006.jpg");
        } catch (IOException e) {
            e.printStackTrace();
        }
        // model init
        File sdDir = Environment.getExternalStorageDirectory();//获取跟目录
        String sdPath = sdDir.toString() + "/siamtracker/";
        siamtrackerInitModel(sdPath);
        mTextureView = findViewById(R.id.textureView);
        mTextureView.setSurfaceTextureListener(mTextureListener);
        startBackgroundThread();
        // surfaceView for paint and select
        mSurfaceView = findViewById(R.id.surfaceView);
        mSurfaceView.setZOrderOnTop(true);  // 设置surfaceView在顶层
        mSurfaceView.getHolder().setFormat(PixelFormat.TRANSPARENT); // 设置surfaceView为透明
        mSurfaceHolder = mSurfaceView.getHolder();  // 获取surfaceHolder以便后面画框
        mSurfaceView.setOnTouchListener(mOntouchListener);
        //
        mPaint = new Paint();  // 写文本的paint
        mPaint.setColor(Color.YELLOW);
        mPaint.setStyle(Paint.Style.STROKE);//不填充
        mPaint.setStrokeWidth(STROKE_WIDTH); //线的宽度
        mCanvas = new Canvas();  // 画布
    }

    @Override
    protected void onPause() {
        closeCamera();
        super.onPause();
    }

    /**
     * 开启处理图片子线程
     */
    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("CameraBackground");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    private void setupCamera(int width, int height) {
        // 获取摄像头的管理者CameraManager
        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            // 遍历所有摄像头
            for (String cameraId : manager.getCameraIdList()) {
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
                // 默认打开后置摄像头 - 忽略前置摄像头
                if (characteristics.get(CameraCharacteristics.LENS_FACING) == CameraCharacteristics.LENS_FACING_FRONT)
                    continue;
                // 获取StreamConfigurationMap，它是管理摄像头支持的所有输出格式和尺寸
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                mPreviewSize = getOptimalSize(map.getOutputSizes(SurfaceTexture.class), width, height);
                int orientation = getResources().getConfiguration().orientation;
                if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
                    mTextureView.setAspectRatio(mPreviewSize.getWidth(), mPreviewSize.getHeight());
                } else {
                    mTextureView.setAspectRatio(mPreviewSize.getHeight(), mPreviewSize.getWidth());
                }


                mCameraId = cameraId;
                break;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void openCamera() {
        //获取相机的管理者CameraManager
        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        //检查权限
        try {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                return;
            }
            //打开相机，第一个参数指示打开哪个摄像头，第二个参数stateCallback为相机的状态回调接口，第三个参数用来确定Callback在哪个线程执行，为null的话就在当前线程执行
            manager.openCamera(mCameraId, mStateCallback, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void closeCamera() {
        if (null != mCaptureSession) {
            mCaptureSession.close();
            mCaptureSession = null;
        }
        if (null != mCameraDevice) {
            mCameraDevice.close();
            mCameraDevice = null;
        }
        if (null != mImageReader) {
            mImageReader.close();
            mImageReader = null;
        }
    }

//    private void configureTransform(int viewWidth, int viewHeight) {
//        if (null == mTextureView || null == mPreviewSize) {
//            return;
//        }
//        int rotation = getWindowManager().getDefaultDisplay().getRotation();
//        Matrix matrix = new Matrix();
//        RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
//        RectF bufferRect = new RectF(0, 0, mPreviewSize.getHeight(), mPreviewSize.getWidth());
//        float centerX = viewRect.centerX();
//        float centerY = viewRect.centerY();
//        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
//            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
//            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
//            float scale = Math.max(
//                    (float) viewHeight / mPreviewSize.getHeight(),
//                    (float) viewWidth / mPreviewSize.getWidth());
//            matrix.postScale(scale, scale, centerX, centerY);
//            matrix.postRotate(90 * (rotation - 2), centerX, centerY);
//        } else if (Surface.ROTATION_180 == rotation) {
//            matrix.postRotate(180, centerX, centerY);
//        }
//        mTextureView.setTransform(matrix);
//    }

    private void startPreview() {
        setupImageReader();
        SurfaceTexture mSurfaceTexture = mTextureView.getSurfaceTexture();
        //设置TextureView的缓冲区大小
        mSurfaceTexture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        //获取Surface显示预览数据
        mPreviewSurface = new Surface(mSurfaceTexture);
        try {
            getPreviewRequestBuilder();
            //创建相机捕获会话，第一个参数是捕获数据的输出Surface列表，第二个参数是CameraCaptureSession的状态回调接口，当它创建好后会回调onConfigured方法，第三个参数用来确定Callback在哪个线程执行，为null的话就在当前线程执行
            mCameraDevice.createCaptureSession(Arrays.asList(mPreviewSurface, mImageReader.getSurface()), new CameraCaptureSession.StateCallback() {
                @Override
                public void onConfigured(CameraCaptureSession session) {
                    mCaptureSession = session;
                    repeatPreview();
                }

                @Override
                public void onConfigureFailed(CameraCaptureSession session) {

                }
            }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void repeatPreview() {
        mPreviewRequestBuilder.setTag(TAG_PREVIEW);
        mPreviewRequest = mPreviewRequestBuilder.build();
        //设置反复捕获数据的请求，这样预览界面就会一直有数据显示
        try {
            mCaptureSession.setRepeatingRequest(mPreviewRequest, null, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private final static int EXECUTION_FREQUENCY = 5;
    private int PREVIEW_RETURN_IMAGE_COUNT = 0;

    private void setupImageReader() {
        Log.i(TAG, "setupImageReader: build image reader");
        //前三个参数分别是需要的尺寸和格式，最后一个参数代表每次最多获取几帧数据
        mImageReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.YUV_420_888, 1);
        //监听ImageReader的事件，当有图像流数据可用时会回调onImageAvailable方法，它的参数就是预览帧数据，可以对这帧数据进行处理
        mImageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
            @Override
            public void onImageAvailable(ImageReader reader) {
//                long beginTime=System.currentTimeMillis();
//                Log.i(TAG,"read begin time: "+beginTime);
                final Image image = reader.acquireLatestImage();
                PREVIEW_RETURN_IMAGE_COUNT++;
                if (PREVIEW_RETURN_IMAGE_COUNT % EXECUTION_FREQUENCY != 0) {
                    image.close();
                    return;
                }
                PREVIEW_RETURN_IMAGE_COUNT = 0;
                if (mStartInit) {
                    //get byte data
                    byte[] yuvBytes = yuv2byte(image);
                    Log.i(TAG, "onImageAvailable: origin init rect " + mInitRect.left + " " +
                            mInitRect.top + " " + mInitRect.right + " " + mInitRect.bottom);
                    float[] initBbox = getInitBBox(mInitRect);
                    Log.i(TAG, "onImageAvailable: changed init rect" + initBbox[0] + " " +
                            initBbox[1] + " " + initBbox[2] + " " + initBbox[3]);
                    siamtrackerInit(yuvBytes, image.getWidth(), image.getHeight(), initBbox);
                    mStartTrack = true;
                    mStartInit = false;
                } else if (mStartTrack) {
                    byte[] yuvBytes = yuv2byte(image);
//                    long process_time=System.currentTimeMillis();
//                    Log.i(TAG,"process_time: "+(process_time));
//                    Log.i(TAG,"read and convert time: "+(process_time-beginTime));
                    float[] trackBBox = siamtrackerTrack(yuvBytes, image.getWidth(), image.getHeight());
                    Log.i(TAG, "onImageAvailable: origin track rect: " + trackBBox[0] + " " +
                            trackBBox[1] + " " + trackBBox[2] + " " + trackBBox[3]);
                    mTrackRect = getTrackBBox(trackBBox);
                    Log.i(TAG, "onImageAvailable: changed track rect: " + mTrackRect.left + " " +
                            mTrackRect.top + " " + mTrackRect.right + " " + mTrackRect.bottom);
                    clearDraw();
                    mCanvas = mSurfaceHolder.lockCanvas();   // 得到surfaceView的画布
                    mCanvas.drawRect(mTrackRect, mPaint);
                    mSurfaceHolder.unlockCanvasAndPost(mCanvas);
                }
                image.close();   // 这里一定要close，不然预览会卡死
//                mBackgroundHandler.post(new Runnable() {    // 在子线程执行，防止预览界面卡顿
//                    @Override
//                    public void run() {
//                        if (mStartInit) {
//                            //get byte data
//                            byte[] yuvBytes = yuv2byte(image);
//                            float[] initBbox = getInitBBox(mInitRect);
//                            siamtrackerInit(yuvBytes, image.getWidth(), image.getHeight(), initBbox);
//                            mStartTrack = true;
//                            mStartInit = false;
//                        }else if (mStartTrack) {
//                            byte[] yuvBytes = yuv2byte(image);
//                            float[] trackBBox = siamtrackerTrack(yuvBytes, image.getWidth(), image.getHeight());
//                            mTrackRect = getTrackBBox(trackBBox);
//                            clearDraw();
//                            mCanvas = mSurfaceHolder.lockCanvas();   // 得到surfaceView的画布
//                            mCanvas.drawRect(mTrackRect, mPaint);
//                            mSurfaceHolder.unlockCanvasAndPost(mCanvas);
//                        }
//                        image.close();   // 这里一定要close，不然预览会卡死
//                    }
//                });
            }
        }, null);
    }

    // 选择sizeMap中大于并且最接近width和height的size
    private Size getOptimalSize(Size[] sizeMap, int width, int height) {
        List<Size> sizeList = new ArrayList<>();
        for (Size option : sizeMap) {
            if (width > height) {
                if (option.getWidth() > width && option.getHeight() > height) {
                    sizeList.add(option);
                }
            } else {
                if (option.getWidth() > height && option.getHeight() > width) {
                    sizeList.add(option);
                }
            }
        }
        if (sizeList.size() > 0) {
            return Collections.min(sizeList, new Comparator<Size>() {
                @Override
                public int compare(Size lhs, Size rhs) {
                    return Long.signum(lhs.getWidth() * lhs.getHeight() - rhs.getWidth() * rhs.getHeight());
                }
            });
        }
        return sizeMap[0];
    }


    // 创建预览请求的Builder（TEMPLATE_PREVIEW表示预览请求）
    private void getPreviewRequestBuilder() {
        try {
            mPreviewRequestBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        //设置预览的显示界面
        Log.i(TAG, "getPreviewRequestBuilder: add target");
        mPreviewRequestBuilder.addTarget(mPreviewSurface);
        mPreviewRequestBuilder.addTarget(mImageReader.getSurface());
        MeteringRectangle[] meteringRectangles = mPreviewRequestBuilder.get(CaptureRequest.CONTROL_AF_REGIONS);
        if (meteringRectangles != null && meteringRectangles.length > 0) {
            Log.d(TAG, "PreviewRequestBuilder: AF_REGIONS=" + meteringRectangles[0].getRect().toString());
        }
        //自动对焦
        mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
        //设置自动曝光帧率范围
        mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, getRange());
//        mPreviewRequestBuilder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO);
        mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_IDLE);
    }

    private Range<Integer> getRange() {
        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        CameraCharacteristics chars = null;
        try {
            chars = manager.getCameraCharacteristics(mCameraId);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        Range<Integer>[] ranges = chars.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);

        Range<Integer> result = null;

        for (Range<Integer> range : ranges) {
            //帧率不能太低，大于10
            if (range.getLower() < 10)
                continue;
            if (result == null)
                result = range;
                //FPS下限小于15，弱光时能保证足够曝光时间，提高亮度。range范围跨度越大越好，光源足够时FPS较高，预览更流畅，光源不够时FPS较低，亮度更好。
            else if (range.getLower() <= 15 && (range.getUpper() - range.getLower()) > (result.getUpper() - result.getLower()))
                result = range;
        }
        return result;
    }

    private void clearDraw() {
        try {
            mCanvas = mSurfaceHolder.lockCanvas(null);
            mCanvas.drawColor(Color.WHITE);
            mCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.SRC);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (mCanvas != null) {
                mSurfaceHolder.unlockCanvasAndPost(mCanvas);
            }
        }
    }

    private void transformBBox(Rect rect) {
        if (rect.left > rect.right) {
            int tmp = rect.left;
            rect.left = rect.right;
            rect.right = tmp;
        }
        if (rect.top > rect.bottom) {
            int tmp = rect.top;
            rect.top = rect.bottom;
            rect.bottom = tmp;
        }
    }

    private float[] getInitBBox(Rect rect) {
        float x1 = rect.top;
        float y1 = 1080 - rect.right;
        float x2 = rect.bottom;
        float y2 = 1080 - rect.left;
        float[] ret = {(x1 + x2) / 2, (y1 + y2) / 2, (x2 - x1), (y2 - y1)};
        return ret;
    }

    private Rect getTrackBBox(float[] bbox) {
        Rect rect = new Rect(0, 0, 0, 0);
        rect.top = (int) (bbox[0] - bbox[2] / 2);
        rect.bottom = (int) (bbox[0] + bbox[2] / 2);
        rect.left = (int) (1080 - bbox[1] - bbox[3] / 2);
        rect.right = (int) (1080 - bbox[1] + bbox[3] / 2);
        return rect;
    }

    private byte[] yuv2byte(Image image) {
        Image.Plane[] planes = image.getPlanes();
        int width = image.getWidth();
        int height = image.getHeight();
        int rowStride = planes[0].getRowStride();
        byte[] yuvBytes = new byte[(int) (width * height * 1.5)];
        //y
        ByteBuffer yBuffer = planes[0].getBuffer();
        byte[] yBytes = new byte[yBuffer.capacity()];
        yBuffer.get(yBytes);
        int srcIdx = 0;
        int dstIdx = 0;
        for (int i = 0; i < height; i++) {
            System.arraycopy(yBytes, srcIdx, yuvBytes, dstIdx, width);
            dstIdx += width;
            srcIdx += rowStride;
        }
        //u,v
        ByteBuffer uBuffer = planes[1].getBuffer();
        byte[] uBytes = new byte[uBuffer.capacity()];
        uBuffer.get(uBytes);
        ByteBuffer vBuffer = planes[2].getBuffer();
        byte[] vBytes = new byte[vBuffer.capacity()];
        vBuffer.get(vBytes);
        for (int i = 0; i < height / 2; i++) {
            for (int j = 0; j < width / 2; j++) {
                yuvBytes[dstIdx++] = uBytes[i * rowStride + j * 2];
                yuvBytes[dstIdx++] = vBytes[i * rowStride + j * 2];
            }
        }
        return yuvBytes;
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

    private void copyBigDataToSD(String strOutFileName) throws IOException {
        Log.i(TAG, "start copy file " + strOutFileName);
        File sdDir = Environment.getExternalStorageDirectory();//get root dir
        File file = new File(sdDir.toString() + "/siamtracker/");
        if (!file.exists()) {
            file.mkdirs();
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

    public native void siamtrackerInit(byte[] yuv_bytes, int width, int height, float[] bbox);

    public native float[] siamtrackerTrack(byte[] yuv_bytes, int width, int height);
}
