package se.gu.wiomote.utils;

import android.os.Handler;

public class TimeoutBoolean {
    private final Handler handler;
    private final long timeout;
    private OnUpdate listener;
    private boolean value;

    public TimeoutBoolean(long timeout) {
        this.timeout = timeout;
        this.value = false;
        this.handler = new Handler();
    }

    public void setTrue() {
        value = true;

        if (listener != null) {
            listener.onUpdate(true);
        }

        handler.removeCallbacksAndMessages(null);
        handler.postDelayed(() -> {
            value = false;

            if (listener != null) {
                listener.onUpdate(false);
            }
        }, timeout);
    }

    public boolean getValue() {
        return value;
    }

    public void setOnUpdateListener(OnUpdate listener) {
        this.listener = listener;
    }

    public interface OnUpdate {
        void onUpdate(boolean value);
    }
}
