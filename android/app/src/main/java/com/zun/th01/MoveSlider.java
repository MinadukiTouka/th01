package com.zun.th01;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;

/**
 * A two-zone horizontal movement slider.  Touching the left half moves left,
 * the right half moves right; releasing releases the key.  Touches outside the
 * view never reach this control.
 */
public class MoveSlider extends View {
    public interface Listener {
        void onDirection(int direction);
    }

    private final Paint bgPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint trackPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint knobPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF rect = new RectF();
    private int direction = 0;
    private Listener listener;

    public MoveSlider(Context context) {
        super(context);
        setClickable(true);
        setFocusable(true);
        bgPaint.setColor(0x55000000);
        trackPaint.setColor(0x88FFFFFF);
        trackPaint.setStrokeCap(Paint.Cap.ROUND);
        knobPaint.setColor(0xFFE8D8A0);
        textPaint.setColor(Color.WHITE);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setTextSize(dp(26));
    }

    public void setListener(Listener l) {
        listener = l;
    }

    private float dp(float v) {
        return v * getResources().getDisplayMetrics().density;
    }

    private void setDirection(int dir) {
        if (direction == dir) return;
        direction = dir;
        if (listener != null) listener.onDirection(dir);
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        float w = getWidth();
        float h = getHeight();
        float pad = dp(4);
        rect.set(pad, pad, w - pad, h - pad);
        canvas.drawRoundRect(rect, dp(14), dp(14), bgPaint);

        float trackY = h * 0.52f;
        float trackLeft = w * 0.27f;
        float trackRight = w * 0.73f;
        trackPaint.setStrokeWidth(dp(6));
        canvas.drawLine(trackLeft, trackY, trackRight, trackY, trackPaint);

        float knobX = w * 0.5f;
        if (direction < 0) knobX = trackLeft + dp(8);
        if (direction > 0) knobX = trackRight - dp(8);
        canvas.drawCircle(knobX, trackY, dp(15), knobPaint);

        float textY = h * 0.87f;
        canvas.drawText("←", w * 0.16f, textY, textPaint);
        canvas.drawText("→", w * 0.84f, textY, textPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE: {
                float x = e.getX();
                setDirection(x < getWidth() * 0.5f ? -1 : 1);
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                setDirection(0);
                performClick();
                return true;
            default:
                return true;
        }
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }
}
