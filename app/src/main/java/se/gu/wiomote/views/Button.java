package se.gu.wiomote.views;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.button.MaterialButton;

public class Button extends MaterialButton {
    private OnEnable listener;

    public Button(@NonNull Context context) {
        super(context);
    }

    public Button(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public Button(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);

        if (listener != null) {
            listener.onEnabledChanged(enabled);
        }
    }

    public void setOnEnableListener(OnEnable listener) {
        this.listener = listener;
    }

    public interface OnEnable {
        void onEnabledChanged(boolean enabled);
    }
}
