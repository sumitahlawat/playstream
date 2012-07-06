package my.streamplayer;

public class Rtsplayer {
	
	static {
	    System.loadLibrary("ffmpeg");
	    System.loadLibrary("rtsp");
	  }
	
	public native void CreateRec(byte[] url ,byte[] recfile, int a, int b, int c, int d);
	public native void StartRec(int a);
	public native void StopRec(int a);
	public native void DestroyRec(int a);		
}
