package my.streamplayer;

import android.app.Activity;
import android.os.Bundle;

public class PlaystreamActivity extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setContentView(R.layout.main);
        Rtsplayer player = new Rtsplayer();
//        String url = "rtsp://ahlawat.servehttp.com/live.sdp";
        String url = "rtsp://180.176.17.24/live.sdp";
        
        //String url = "rtsp://192.168.11.12:8554/test.m4e";
        String recfile = "/mnt/sdcard/record.mov";
        player.CreateRec(url, recfile, 1, 10, 10, 30);
        player.StartRec(1);
    }
}