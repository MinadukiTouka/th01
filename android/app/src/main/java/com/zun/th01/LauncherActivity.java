package com.zun.th01;

import android.app.Activity;
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Copies the bundled TH01 data into the app's external files folder, then
 * starts the SDL2 game activity.
 */
public class LauncherActivity extends Activity {
    private static final String TAG = "th01";

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);

        TextView tv = new TextView(this);
        tv.setText("東方靈異伝\n\n准备数据…");
        tv.setTextSize(22f);
        tv.setPadding(48, 48, 48, 48);
        setContentView(tv);

        new Thread(() -> {
            try {
                File dir = new File(getExternalFilesDir(null), "th01");
                if (!dir.isDirectory() && !dir.mkdirs()) {
                    throw new IOException("cannot create " + dir);
                }
                copyAssets("th01", dir);
                runOnUiThread(() -> {
                    startActivity(new Intent(this, GameActivity.class));
                    finish();
                });
            } catch (Exception e) {
                Log.e(TAG, "data import failed", e);
                runOnUiThread(() -> tv.setText("数据导入失败：\n" + e));
            }
        }).start();
    }

    private void copyAssets(String assetDir, File outDir) throws IOException {
        AssetManager am = getAssets();
        String[] names = am.list(assetDir);
        if (names == null) return;
        for (String name : names) {
            String assetPath = assetDir + "/" + name;
            String[] children = am.list(assetPath);
            if (children != null && children.length > 0) {
                File sub = new File(outDir, name);
                if (!sub.isDirectory() && !sub.mkdirs()) throw new IOException("mkdir " + sub);
                copyAssets(assetPath, sub);
                continue;
            }
            File out = new File(outDir, name);
            if (out.isFile() && out.length() > 0) continue;
            try (InputStream in = am.open(assetPath);
                 OutputStream os = new FileOutputStream(out)) {
                byte[] buf = new byte[64 * 1024];
                int n;
                while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
            }
        }
    }
}
