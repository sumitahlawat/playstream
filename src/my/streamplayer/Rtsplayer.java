package my.streamplayer;

import android.graphics.Bitmap;

public class Rtsplayer {
	
	static {
	    System.loadLibrary("ffmpeg");
	    System.loadLibrary("rtsp");
	  }
	//realted to rendering
	
	public static native void native_gl_resize(int w, int h);
	private static native void native_gl_render();
	
	public native void CreateRec(Bitmap mBitmap, String url ,String recfile, int a, int b, int c, int d);
	public native void StartRec(int a);
	public native void StopRec(int a);
	public native void DestroyRec(int a);		
}
