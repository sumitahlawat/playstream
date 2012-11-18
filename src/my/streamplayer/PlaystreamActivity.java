package my.streamplayer;

import java.io.File;
import java.io.FileOutputStream;
import java.util.Calendar;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Toast;

public class PlaystreamActivity extends Activity {
	/** Called when the activity is first created. */

	private static final String TAG = "Playstream";

	private Button btn_save;
	private Button btn_stop;	
	private Button btn_exit;
	private EditText url_text;
	private EditText Xres;
	private EditText Yres;

	private Bitmap mBitmap1;
	Rtsplayer rtplayer = new Rtsplayer(this);
	private Button btn_play2;
	private Button btn_play1;

	private MyPanelView mv;

	@Override 
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//		super.onCreate(null);
		setContentView(R.layout.main);         		
		Display display = getWindowManager().getDefaultDisplay();

		Log.v("Playstream", "height = "+ display.getHeight());
		Log.v("Playstream", "width = "+ display.getWidth());     

		Xres = (EditText) findViewById(R.id.editTextxres);
		Xres.setText("320");
		Yres = (EditText) findViewById(R.id.editTextyres);
		Yres.setText("200");
		url_text = (EditText) findViewById(R.id.editText1);
		url_text.setText("rtsp://192.168.101.102/live2.sdp");
		//url_text.setText("rtsp://ahlawat.servehttp.com/live.sdp");

		File directory = new File(Environment.getExternalStorageDirectory()+File.separator+"ipcam");
		directory.mkdirs();

		ImageView image = (ImageView)findViewById(R.id.frame1);
		image.setBackgroundColor(Color.YELLOW);
		ImageView image2 = (ImageView)findViewById(R.id.frame2);
		image2.setBackgroundColor(Color.BLUE);
		image2.setVisibility(View.GONE);

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

				Log.v("Playstream", "bitmap resize : " + mBitmap1.getWidth() + "  "+mBitmap1.getHeight());
				String recfile = Environment.getExternalStorageDirectory().toString()+"/ipcam/record1.mov";
				String url = url_text.getText().toString();
				rtplayer.CreateRec(url, recfile, 1, Integer.parseInt(Xres.getText().toString()), Integer.parseInt(Yres.getText().toString()), 30 ,mBitmap1);
				rtplayer.StartRec(1);
				btn_play1.setEnabled(true);
			}
		});

		btn_play2 = (Button) findViewById(R.id.button5);
		btn_play2.setText("Play2");
		btn_play2.setVisibility(View.GONE);

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
				finish();
			}
		});		
	}

	public void showbitmap1()
	{		
		ImageView i = (ImageView)findViewById(R.id.frame1);
		i.setImageBitmap(mBitmap1);
		i.refreshDrawableState();			
		
		MyPanelView Pview = (MyPanelView)findViewById(R.id.Panel1);	
		Pview.mBitmap=mBitmap1	;
		Pview.render();
	}

	public Bitmap getbitmap1()
	{
		return mBitmap1;
	}	
}

class MyPanelView extends SurfaceView implements SurfaceHolder.Callback {

	private static final String TAG = "MyPanelView";

	public static Bitmap mBitmap;	
	private boolean toggle = true;
	private PlaystreamActivity act;

	private MjpegViewThread mThread;

	public final static int SIZE_STANDARD = 1;
	public final static int SIZE_FIXED = 2;
	public final static int SIZE_BEST_FIT = 4;
	public final static int SIZE_FULLSCREEN = 8;

	private boolean mRun = false;	

	private boolean surfaceDone = false;
	private int dispWidth;
	private int dispHeight;
	private int displayMode;

	public MyPanelView(Context context) {
		super(context);
		init(context);
	}

	public MyPanelView(Context context, AttributeSet attr) {
		super(context, attr);
		init(context);
	}

	public void setactivity(PlaystreamActivity activity)
	{
		act=activity;
	}

	public void startPlayback() {
		Log.d(TAG, "startPlayback ");
		/*if(mIn != null)*/ {
			mRun = true;
			mThread.start();
		}
	}

	public void setDisplayMode(int s) {
		displayMode = s;
	}

	public void doDraw(Canvas canvas) {
		Bitmap Bmap = act.getbitmap1();
		Log.v("Playstream", " todraw : "+Bmap.toString());
		Paint p =new Paint();
		canvas.drawColor(Color.BLUE);
		canvas.drawBitmap(Bmap, 0, 0, p);
		mThread.todraw=false; //draw done, wait for new bitmap
	}

	public void render()
	{		
		mThread.todraw=true; //now can draw
	}

	public void render(Bitmap bmap)
	{
		mThread.todraw=true; //now can draw
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		mThread.setSurfaceSize(width, height);
	}

	public void surfaceCreated(SurfaceHolder holder) {
		surfaceDone=true;
	}

	public void surfaceDestroyed(SurfaceHolder holder) {

	}

	private void init(Context context) {
		SurfaceHolder holder = getHolder();
		holder.addCallback(this);
		mThread = new MjpegViewThread(holder, context);
		setFocusable(true);
		displayMode = MyPanelView.SIZE_FIXED;
		dispWidth = getWidth();
		dispHeight = getHeight();
	}

	public class MjpegViewThread extends Thread {
		private SurfaceHolder mSurfaceHolder;	
		public boolean todraw=false;

		public MjpegViewThread(SurfaceHolder surfaceHolder, Context context) {
			mSurfaceHolder = surfaceHolder;
		}

		private Rect destRect(int bmw, int bmh) {
			int tempx;
			int tempy;
			if (displayMode == MyPanelView.SIZE_FIXED){
				return new Rect(0, 0, bmw, bmh);
			}
			if (displayMode == MyPanelView.SIZE_STANDARD) {
				tempx = (dispWidth / 2) - (bmw / 2);
				tempy = (dispHeight / 2) - (bmh / 2);
				return new Rect(tempx, tempy, bmw + tempx, bmh + tempy);
			}
			if (displayMode == MyPanelView.SIZE_BEST_FIT) {
				float bmasp = (float) bmw / (float) bmh;
				bmw = dispWidth;
				bmh = (int) (dispWidth / bmasp);
				if (bmh > dispHeight) {
					bmh = dispHeight;
					bmw = (int) (dispHeight * bmasp);
				}
				tempx = (dispWidth / 2) - (bmw / 2);
				tempy = (dispHeight / 2) - (bmh / 2);
				return new Rect(tempx, tempy, bmw + tempx, bmh + tempy);
			}
			if (displayMode == MyPanelView.SIZE_FULLSCREEN){
				return new Rect(0, 0, dispWidth, dispHeight);
			}
			return null;
		}

		public void setSurfaceSize(int width, int height) {
			synchronized(mSurfaceHolder) {
				dispWidth = width;
				dispHeight = height;
			}
		}


		public void run() {
			Log.d(TAG, "thread start");	
			Bitmap bm;
			Rect destRect;
			Canvas c = null;
			Paint p = new Paint();	
			while (mRun) {
				if(surfaceDone) {
					try {
						c = mSurfaceHolder.lockCanvas();
						synchronized (mSurfaceHolder) {
							try {							
								bm = MyPanelView.mBitmap;
								destRect = destRect(bm.getWidth(),bm.getHeight());
								Log.d(TAG, "draw :" + destRect.width());
								Log.d(TAG, "new frame : "+ destRect.height());
								c.drawColor(Color.BLACK);
								c.drawBitmap(bm, null, destRect, p);
							} catch (Exception e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
						}
					} finally {
						if (c != null) {
							mSurfaceHolder.unlockCanvasAndPost(c);
						}
					}
				}
			}
		}
	}
}