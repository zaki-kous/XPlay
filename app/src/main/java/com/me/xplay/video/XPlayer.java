package com.me.xplay.video;

/**
 * video player
 *
 * Created by zhuqian on 17/10/10.
 */

public class XPlayer {

    static {
        System.loadLibrary("xplay");
    }


    public native int openVideo(String url);
}
