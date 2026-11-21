package com.zun.th01;

import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import org.libsdl.app.SDLActivity;

public class GameActivity extends SDLActivity {
    private static final int DP = 1;
    private View saveArrows;
    private final Handler saveHandler = new Handler(Looper.getMainLooper());
    private final Runnable savePoll = new Runnable() {
        @Override public void run() {
            if (saveArrows != null) {
                saveArrows.setVisibility(nativeSaveMenuActive() ? View.VISIBLE : View.GONE);
            }
            saveHandler.postDelayed(this, 150);
        }
    };

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        hideSystemUi();
        buildOverlay();
    }

    private int dp(int v) {
        return (int) (v * getResources().getDisplayMetrics().density);
    }

    private TextView button(String label, int w, int h) {
        TextView b = new TextView(this);
        b.setText(label);
        b.setTextColor(Color.WHITE);
        b.setTextSize(22f);
        b.setGravity(Gravity.CENTER);
        b.setBackgroundColor(0x55000000);
        b.setPadding(dp(4), dp(2), dp(4), dp(2));
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(dp(w), dp(h));
        b.setLayoutParams(lp);
        return b;
    }

    private void hold(TextView b, final int group, final int bits) {
        b.setOnTouchListener((v, e) -> {
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    nativeKey(group, bits);
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                case MotionEvent.ACTION_POINTER_UP:
                    nativeKey(group, 0);
                    return true;
                default:
                    return true;
            }
        });
    }

    private void buildOverlay() {
        View content = SDLActivity.getContentView();
        if (!(content instanceof RelativeLayout)) return;
        RelativeLayout root = (RelativeLayout) content;

        SharedPreferences prefs = getSharedPreferences("port", MODE_PRIVATE);
        boolean leftHanded = prefs.getBoolean("left_handed", false);

        int bottom = dp(14);
        int side = dp(16);
        int bw = 86;
        int bh = 78;

        // TH01 only uses left/right for movement.  A single slider replaces
        // the four D-pad buttons; touches outside this view are ignored.
        MoveSlider slider = new MoveSlider(this);
        int sw = 156;
        int sh = 110;
        RelativeLayout.LayoutParams slp =
            new RelativeLayout.LayoutParams(dp(sw), dp(sh));
        slp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        if (leftHanded) {
            slp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
            slp.rightMargin = side;
        } else {
            slp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
            slp.leftMargin = side;
        }
        slp.bottomMargin = bottom;
        slider.setLayoutParams(slp);
        slider.setListener(direction ->
            nativeKey(7, direction < 0 ? 0x08 : (direction > 0 ? 0x10 : 0))
        );
        root.addView(slider);

        TextView z = button("Z", bw, bh);
        TextView x = button("X", bw, bh);
        TextView bomb = button("B", bw, bh);
        TextView esc = button("Esc", bw, bh);
        RelativeLayout.LayoutParams zlp = (RelativeLayout.LayoutParams) z.getLayoutParams();
        RelativeLayout.LayoutParams xlp = (RelativeLayout.LayoutParams) x.getLayoutParams();
        RelativeLayout.LayoutParams blp = (RelativeLayout.LayoutParams) bomb.getLayoutParams();
        RelativeLayout.LayoutParams elp = (RelativeLayout.LayoutParams) esc.getLayoutParams();
        zlp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        xlp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        blp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        elp.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        if (leftHanded) {
            zlp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
            zlp.leftMargin = side;
            xlp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
            xlp.leftMargin = side + dp(bw) + dp(8);
            blp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
            blp.leftMargin = side + (dp(bw) + dp(8)) * 2;
            elp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
            elp.rightMargin = side;
        } else {
            zlp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
            zlp.rightMargin = side;
            xlp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
            xlp.rightMargin = side + dp(bw) + dp(8);
            blp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
            blp.rightMargin = side + (dp(bw) + dp(8)) * 2;
            elp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
            elp.leftMargin = side;
        }
        zlp.bottomMargin = bottom;
        xlp.bottomMargin = bottom;
        blp.bottomMargin = bottom;
        elp.topMargin = dp(14);
        root.addView(z);
        root.addView(x);
        root.addView(bomb);
        root.addView(esc);

        // Save/name-entry screen only: up/down for the high-score table.
        LinearLayout arrows = new LinearLayout(this);
        arrows.setOrientation(LinearLayout.VERTICAL);
        arrows.setGravity(Gravity.CENTER);
        TextView up = button("▲", 64, 60);
        TextView down = button("▼", 64, 60);
        up.setLayoutParams(new LinearLayout.LayoutParams(dp(64), dp(60)));
        down.setLayoutParams(new LinearLayout.LayoutParams(dp(64), dp(60)));
        up.setTextSize(26f);
        down.setTextSize(26f);
        arrows.addView(up);
        arrows.addView(down);
        RelativeLayout.LayoutParams alp = new RelativeLayout.LayoutParams(dp(64), dp(132));
        alp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        alp.addRule(RelativeLayout.CENTER_VERTICAL);
        alp.leftMargin = side;
        arrows.setLayoutParams(alp);
        arrows.setVisibility(View.GONE);
        root.addView(arrows);
        saveArrows = arrows;
        hold(up, 7, 0x04);
        hold(down, 7, 0x20);
        saveHandler.post(savePoll);

        hold(z, 5, 0x02);
        hold(x, 5, 0x04);
        bomb.setOnTouchListener((v, e) -> {
            if (e.getActionMasked() == MotionEvent.ACTION_DOWN) nativeBomb();
            return true;
        });
        esc.setOnTouchListener((v, e) -> {
            if (e.getActionMasked() == MotionEvent.ACTION_DOWN) nativeKey(0, 0x01);
            if (e.getActionMasked() == MotionEvent.ACTION_UP) nativeKey(0, 0);
            return true;
        });
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) nativeKey(0, 0x01);
            if (event.getAction() == KeyEvent.ACTION_UP) nativeKey(0, 0);
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();
    }

    private void hideSystemUi() {
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
    }

    @Override
    protected void onPause() {
        saveHandler.removeCallbacks(savePoll);
        if (saveArrows != null) saveArrows.setVisibility(View.GONE);
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        saveHandler.removeCallbacks(savePoll);
        saveHandler.post(savePoll);
    }

    private static native void nativeKey(int group, int bits);
    private static native void nativeBomb();
    private static native boolean nativeSaveMenuActive();
}
