package se.gu.wiomote.views;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.util.AttributeSet;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class CircleLinearLayout extends LinearLayout {
    private Path path;

    public CircleLinearLayout(@NonNull Context context) {
        super(context);

        init();
    }

    public CircleLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        init();
    }

    public CircleLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        init();
    }

    private void init() {
        path = new Path();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        if (getMeasuredHeight() != getMeasuredWidth()) {
            int size = Math.min(getMeasuredHeight(), getMeasuredWidth());

            getLayoutParams().height = size;
            getLayoutParams().width = size;

            path.reset();
            path.addOval(0, 0, size, size, Path.Direction.CW);

            invalidate();
        }
    }

    @Override
    protected void dispatchDraw(@NonNull Canvas canvas) {
        canvas.clipPath(path);

        super.dispatchDraw(canvas);
    }
}
