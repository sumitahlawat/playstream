package my.streamplayer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class PlaystreamActivity extends Activity {
	/** Called when the activity is first created. */

	private Button btn_start;
	private Button btn_stop;
	private Button btn_play;
	private Button btn_exit;
	private EditText url_text;	
	private GLSurfaceView surfaceView;

	@Override 
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//		super.onCreate(null);
		Log.v("Playstream", "before content set");
		setContentView(R.layout.main);         
		Log.v("Playstream", "set content done");
		Display display = getWindowManager().getDefaultDisplay();        

		Log.v("Playstream", "height = "+ display.getHeight());
		Log.v("Playstream", "width = "+ display.getWidth());     

		url_text = (EditText) findViewById(R.id.editText1);
		//		url_text.setText("rtsp://tijuana.ucsd.edu/branson/physics130a/spring2003/060203_full.mp4");
		url_text.setText("rtsp://ahlawat.servehttp.com/live.sdp");
		final Rtsplayer player = new Rtsplayer();

		btn_start = (Button) findViewById(R.id.button1);	
		btn_start.setText("record");
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
		btn_play.setText("Play");
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
		btn_stop.setText("Stop");
		btn_stop.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_stop.setEnabled(false);				
				player.StopRec(1);
				btn_stop.setEnabled(true);
			}
		});

		btn_exit = (Button) findViewById(R.id.button2);
		btn_exit.setText("Quit");
		btn_exit.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_exit.setEnabled(false);
				finish();
			}
		});		
		surfaceView = (GLSurfaceView) findViewById(R.id.glSurface);
		surfaceView.setRenderer(new MyRenderer());
	}
}


class MyGLSurfaceView extends GLSurfaceView {
	MyRenderer mRenderer;

	public MyGLSurfaceView(Context context) {
		super(context);
		mRenderer = new MyRenderer();
		setRenderer(mRenderer);
	}

	public boolean onTouchEvent(final MotionEvent event) {
		queueEvent(new Runnable(){
			public void run() {
				
			}});
		return true;
	}
}

class MyRenderer implements GLSurfaceView.Renderer {
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		// Do nothing special.
	}

	public void onSurfaceChanged(GL10 gl, int w, int h) {
		gl.glViewport(0, 0, w, h);
	}

	public void onDrawFrame(GL10 gl) {
		gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
	}
}