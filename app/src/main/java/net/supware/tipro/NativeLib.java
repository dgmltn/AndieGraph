package net.supware.tipro;

public class NativeLib {
	private static final String TAG = NativeLib.class.getSimpleName();
	private static final int IO_BUFFER_SIZE = 4 * 1024;

	// load our native library
	static {
		System.loadLibrary("ti8x");
	}

	// implemented by libti8x.so
	public static native void onResume();

	public static native void onPause();

	public static native void keyDown(int key);

	public static native void keyUp(int key);

	//public static native void loadState(String ramFilename);

	//public static native void saveState(String ramFilename);

	public static native void renderScreen(int[] bitmap);

	public static native void start(int modelId, String romFilename, String ramFilename);

	public static native void stop();
}
