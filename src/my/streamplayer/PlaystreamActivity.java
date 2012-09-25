package my.streamplayer;

import java.io.File;
import java.io.FileOutputStream;
import java.util.Calendar;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Toast;

public class PlaystreamActivity extends Activity {
	/** Called when the activity is first created. */

	private Button btn_save;
	private Button btn_stop;	
	private Button btn_exit;
	private EditText url_text;
	private EditText Xres;
	private EditText Yres;
	//private MyGLSurfaceView surfaceView;
	private Bitmap mBitmap1;
	private Bitmap mBitmap2;
	private Bitmap mBitmap3;
	private Bitmap mBitmap4;
	Rtsplayer rtplayer = new Rtsplayer(this);
	private Button btn_play2;
	private Button btn_play1;
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

		Xres = (EditText) findViewById(R.id.editTextxres);
		Xres.setText("320");
		Yres = (EditText) findViewById(R.id.editTextyres);
		Yres.setText("240");
		url_text = (EditText) findViewById(R.id.editText1);
		//		url_text.setText("rtsp://tijuana.ucsd.edu/branson/physics130a/spring2003/060203_full.mp4");		
		url_text.setText("rtsp://ahlawat.servehttp.com/");

		Log.v("Playstream", "imageview scaling done");


		File directory = new File(Environment.getExternalStorageDirectory()+File.separator+"ipcam");
		directory.mkdirs();

		ImageView image = (ImageView)findViewById(R.id.frame1);
		image.setBackgroundColor(Color.GREEN);		
		ImageView image2 = (ImageView)findViewById(R.id.frame2);
		image2.setBackgroundColor(Color.BLUE);

		btn_play1 = (Button) findViewById(R.id.button4);
		btn_play1.setText("Play1");
		btn_play1.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_play1.setEnabled(false);

				mBitmap1 = Bitmap.createBitmap(Integer.parseInt(Xres.getText().toString()), Integer.parseInt(Yres.getText().toString()), Bitmap.Config.ARGB_8888);

				ImageView image = (ImageView)findViewById(R.id.frame1);
				image.setBackgroundColor(Color.BLACK);
				LinearLayout layout = (LinearLayout)findViewById(R.id.rootid1);
				LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(Integer.parseInt(Xres.getText().toString()), Integer.parseInt(Yres.getText().toString()));
				image.setLayoutParams(params);
				image.setScaleType(ImageView.ScaleType.CENTER_CROP);

				Log.v("Playstream", "image resize : " + image.getWidth() + "  "+image.getHeight());
				Log.v("Playstream", "bitmap resize : " + mBitmap1.getWidth() + "  "+mBitmap1.getHeight());
				String recfile = Environment.getExternalStorageDirectory().toString()+"/ipcam/record1.mov";
				String url = url_text.getText().toString()+"live.sdp";
				rtplayer.CreateRec(url, recfile, 1, Integer.parseInt(Xres.getText().toString()), Integer.parseInt(Yres.getText().toString()), 30 ,mBitmap1);
				rtplayer.StartRec(1);
				btn_play1.setEnabled(true);
			}
		});

		btn_play2 = (Button) findViewById(R.id.button5);
		btn_play2.setText("Play2");
		btn_play2.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_play2.setEnabled(false);

				mBitmap2 = Bitmap.createBitmap(176, 144, Bitmap.Config.ARGB_8888);
				ImageView image = (ImageView)findViewById(R.id.frame2);
				image.setBackgroundColor(Color.BLACK);
				LinearLayout layout = (LinearLayout)findViewById(R.id.rootid2);
				LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(176, 144);
				image.setLayoutParams(params);
				image.setScaleType(ImageView.ScaleType.CENTER_CROP);

				Log.v("Playstream", "image resize : " + image.getWidth() + "  "+image.getHeight());
				Log.v("Playstream", "bitmap resize : " + mBitmap2.getWidth() + "  "+mBitmap2.getHeight());
				String recfile = Environment.getExternalStorageDirectory().toString()+"/ipcam/record2.mov";
				String url = url_text.getText().toString()+"live2.sdp";
				rtplayer.CreateRec(url, recfile, 2, 176, 144, 30 ,mBitmap2);
				rtplayer.StartRec(2);
				btn_play2.setEnabled(true);
			}
		});

		btn_save = (Button) findViewById(R.id.button1);	
		btn_save.setText("Snap1");
		btn_save.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_save.setEnabled(false);
				String filename= Environment.getExternalStorageDirectory().toString()+"/ipcam/"+ Calendar.getInstance().get(Calendar.HOUR_OF_DAY) + Calendar.getInstance().get(Calendar.MINUTE)+Calendar.getInstance().get(Calendar.SECOND) +".jpg";
				try {
					FileOutputStream out = new FileOutputStream(filename);
					mBitmap1.compress(Bitmap.CompressFormat.JPEG, 90, out);
				} catch (Exception e) {
					e.printStackTrace();
				}
				Toast.makeText(getApplicationContext(), "screenshot saved to "+filename, Toast.LENGTH_LONG).show();
				btn_save.setEnabled(true);
			}
		});

		btn_stop = (Button) findViewById(R.id.button3);	
		btn_stop.setText("Stop");
		btn_stop.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_stop.setEnabled(false);				
				rtplayer.StopRec(1);
				ImageView image = (ImageView)findViewById(R.id.frame1);
				image.setBackgroundColor(Color.GREEN);
				rtplayer.StopRec(2);
				ImageView image2 = (ImageView)findViewById(R.id.frame2);
				image2.setBackgroundColor(Color.BLUE);
				btn_stop.setEnabled(true);
			}
		});

		btn_exit = (Button) findViewById(R.id.button2);
		btn_exit.setText("Quit");
		btn_exit.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn_exit.setEnabled(false);
				rtplayer.StopRec(1);
				rtplayer.StopRec(2);
				finish();
			}
		});		
	}

	public void showbitmap1()
	{		
		ImageView i = (ImageView)findViewById(R.id.frame1);
		i.setImageBitmap(mBitmap1);
	}

	public void showbitmap2() {
		// TODO Auto-generated method stub
		ImageView i = (ImageView)findViewById(R.id.frame2);
		i.setImageBitmap(mBitmap2);
	}

	public void showbitmap3() {
		// TODO Auto-generated method stub

	}

	public void showbitmap4() {
		// TODO Auto-generated method stub

	}
}

class MyGLSurfaceView extends GLSurfaceView {
	MyRenderer mRenderer;

	public MyGLSurfaceView(Context context, AttributeSet attrs) {		
		super(context, attrs);

		Log.v("Playstream", "MyGLSurfaceView " + attrs.toString());
		mRenderer = new MyRenderer();
		setRenderer(mRenderer);
		setFocusableInTouchMode(true);
	}		

	public boolean onTouchEvent(final MotionEvent event) {
		queueEvent(new Runnable(){
			public void run() {
				mRenderer.setColor(event.getX() / getWidth(),
						event.getY() / getHeight(), 1.0f);
			}});
		return true;
	}
}

class MyRenderer implements GLSurfaceView.Renderer {		
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {				
		// Do nothing special.
	}

	public void onSurfaceChanged(GL10 gl, int w, int h) {
		Log.d("Playstream", "onSurfaceChanged " + w +"  "+ h);
		gl.glViewport(0, 0, w, h);
		//		Rtsplayer.native_gl_resize(w, h);		
	}

	public void onDrawFrame(GL10 gl) {
		gl.glClearColor(mRed, mGreen, mBlue, 1.0f);
		gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
	}
	public void setColor(float r, float g, float b) {
		mRed = r;
		mGreen = g;
		mBlue = b;
	}

	private float mRed;
	private float mGreen;
	private float mBlue;

}