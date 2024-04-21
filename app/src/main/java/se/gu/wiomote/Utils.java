package se.gu.wiomote;

import android.os.Handler;
import android.os.Looper;

public class Utils {
    private static final Handler handler = new Handler(Looper.getMainLooper());

    public static void runOnUiThread(Runnable runnable) {
        handler.post(runnable);
    }
}
