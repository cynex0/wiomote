package se.gu.wiomote.utils;

import android.os.Handler;

public class TimeoutBoolean {
    private final Handler handler;
    private final Runnable runnable;
    private final long timeout;
    private OnUpdate listener;
    private boolean value;

    public TimeoutBoolean(long timeout) {
        this.timeout = timeout;
        this.value = false;
        this.handler = new Handler();
        this.runnable = () -> {
            value = false;

            if (listener != null) {
                listener.onUpdate(false);
            }
        };
    }

    public void setTrue() {
        value = true;

        if (listener != null) {
            listener.onUpdate(true);
        }

        handler.removeCallbacksAndMessages(null);
        handler.postDelayed(runnable, timeout);
    }

    public boolean getValue() {
        return value;
    }

    public void setOnUpdateListener(OnUpdate listener) {
        this.listener = listener;
    }

    public void forceTimeout() {
        if (value) {
            handler.removeCallbacksAndMessages(null);
            runnable.run();
        }
    }

    public interface OnUpdate {
        void onUpdate(boolean value);
    }
}
