package my.streamplayer;

import java.lang.ref.WeakReference;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class Rtsplayer {
	private static Handler h;
	static {
		System.loadLibrary("ffmpeg");
		System.loadLibrary("rtsp");
	}
	final static int RCM_CB_string=1;
	PlaystreamActivity mainActivity;
	public Rtsplayer(PlaystreamActivity playstreamActivity){
		init(new WeakReference<Rtsplayer>(this));
		mainActivity = playstreamActivity;
		h = new Handler(){
			public void handleMessage(Message msg) {
				switch (msg.what)
				{
				case RCM_CB_string:
					try {
						String mystr= (String) msg.obj;            	   
//						Log.d("Playstream","CB_string   env: " + String.valueOf(msg.arg1)+"  string: " +mystr);
						mainActivity.showbitmap();					
					} catch (Exception e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;

				default:
					break;

				}//switch
			}
		};
		Log.d("Playstream","constructor ... end ............. ");
	}


	@SuppressWarnings("unused")
	private static void rcmcallback_string(String strx){
		Message m = h.obtainMessage(RCM_CB_string, 0, 0, strx);
		h.sendMessage(m);
	}

	//realted to rendering

	//	public static native void native_gl_resize(int w, int h);
	//	private static native void native_gl_render();
	private native void init(Object weak_this);
	public native void CreateRec(String url ,String recfile, int a, int b, int c, int d, Bitmap bitmap);
	public native void StartRec(int a);
	public native void StopRec(int a);
	public native void DestroyRec(int a);		
}
