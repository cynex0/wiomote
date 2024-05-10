package se.gu.wiomote.views;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;

public class CircleConstraintLayout extends ConstraintLayout {
    private Path path;

    public CircleConstraintLayout(@NonNull Context context) {
        super(context);

        init();
    }

    public CircleConstraintLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        init();
    }

    public CircleConstraintLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        init();
    }

    private void init() {
        path = new Path();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        if(getMeasuredWidth() > getMeasuredHeight()) {
            super.onMeasure(heightMeasureSpec, heightMeasureSpec);
        } else {
            super.onMeasure(widthMeasureSpec, widthMeasureSpec);
        }

        int size = Math.min(getMeasuredHeight(), getMeasuredWidth());

        path.reset();
        path.addOval(0, 0, size, size, Path.Direction.CW);

        invalidate();
    }

    @Override
    protected void dispatchDraw(@NonNull Canvas canvas) {
        canvas.clipPath(path);

        super.dispatchDraw(canvas);
    }
}
