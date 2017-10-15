package com.me.xplay;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.widget.TextView;

import com.me.xplay.video.XPlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private XPlayer xPlayer;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (xPlayer == null) {
            xPlayer = new XPlayer();
        }

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        String videoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + "test.mp4";
        tv.setText("open video ret : " + xPlayer.openVideo(videoPath));
    }
}
