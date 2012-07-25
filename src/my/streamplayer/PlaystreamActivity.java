package my.streamplayer;

import android.app.Activity;
import android.graphics.Point;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class PlaystreamActivity extends Activity {
	/** Called when the activity is first created. */

	private Button btn_start;
	private Button btn_stop;
	private Button btn_play;
	private Button btn_exit;
	private EditText url_text;

	@Override 
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.main);

		//        String url = "rtsp://ahlawat.servehttp.com/live.sdp";
		//        String url = "rtsp://180.176.17.24/live.sdp";          

		Display display = getWindowManager().getDefaultDisplay();        

		Log.v("Playstream", "height = "+ display.getHeight());
		Log.v("Playstream", "width = "+ display.getWidth());     
		
		url_text = (EditText) findViewById(R.id.editText1);
		url_text.setText("rtsp://tijuana.ucsd.edu/branson/physics130a/spring2003/060203_full.mp4");
//		url_text.setText("rtsp://ahlawat.servehttp.com/live.sdp");
		final Rtsplayer player = new Rtsplayer();
 
		btn_start = (Button) findViewById(R.id.button1);				
		btn_start.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_start.setEnabled(false);
				String recfile = "/mnt/sdcard/ipcam1/record.mov";
				String url = url_text.getText().toString();
				player.CreateRec(url, recfile, 1, 10, 10, 30);
				player.StartRec(1);
				btn_start.setEnabled(true);
			}
		});

		btn_play = (Button) findViewById(R.id.button4);				
		btn_play.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_play.setEnabled(false);

				String recfile = "/mnt/sdcard/ipcam1/record.mov";
				String url = url_text.getText().toString();
				player.CreateRec(url, recfile, 1, 10, 10, 30);
				player.StartRec(1);
				btn_start.setEnabled(true);
			}
		});

		btn_stop = (Button) findViewById(R.id.button3);				
		btn_stop.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_stop.setEnabled(false);				
				player.StopRec(1);
				btn_stop.setEnabled(true);
			}
		});

		btn_exit = (Button) findViewById(R.id.button2);				
		btn_exit.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_exit.setEnabled(false);
				finish();
			}
		});
	}
}