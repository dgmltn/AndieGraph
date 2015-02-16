package net.supware.tipro.view;

import java.util.LinkedList;
import java.util.Queue;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import net.supware.tipro.NativeLib;
import net.supware.tipro.R;

public class ScreenView extends View {
	private static final String TAG = ScreenView.class.getSimpleName();

	private Bitmap mBitmap, mOverlay;
	private boolean mHasDrawnValidFrame;

	private int mPixelWidth, mPixelHeight;
	private Rect mDstRect, mOverlaySrcRect, mOverlayDstRect;
	private int[] mPixels;

	// Make keypresses more stable by syncing to each screen refresh
	public static class KeyState {
		public int code;
		public boolean pressed;

		public KeyState(int code, boolean pressed) {
			this.code = code;
			this.pressed = pressed;
		}
	}

	public Queue<KeyState> mKeyQueue;

	@TargetApi(11)
	public ScreenView(Context context, AttributeSet attributes) {
		super(context, attributes);

		// Hardware rendering for this view forces it to antialias (and blur)
		// In this specific case, we want sharp pixels.
		if (Build.VERSION.SDK_INT >= 11) {
			setLayerType(View.LAYER_TYPE_SOFTWARE, null);
		}

		mKeyQueue = new LinkedList<KeyState>();
	}

	public void setPixelSize(int width, int height) {
		if (width == mPixelWidth && height == mPixelHeight)
			return;

		mPixelWidth = width;
		mPixelHeight = height;
		mPixels = new int[width * height];
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
	}

	public void setViewPixelsRegion(Rect region) {
		mDstRect = new Rect(region);
	}

	public void clear() {
		mPixelWidth = 0;
		mPixelHeight = 0;
		mBitmap = null;
		mHasDrawnValidFrame = false;
		mPixels = null;
	}

	@Override
	protected void onDraw(Canvas canvas) {
		if (mBitmap == null) {
			drawOverlay(canvas);
			return;
		}

		if (!mKeyQueue.isEmpty()) {
			KeyState key = mKeyQueue.poll();
			if (key.pressed) {
				NativeLib.keyDown(key.code);
			}
			else {
				NativeLib.keyUp(key.code);
			}
		}

		try {
			NativeLib.renderScreen(mPixels);
			mBitmap.setPixels(mPixels, 0, mPixelWidth, 0, 0, mPixelWidth, mPixelHeight);
		}
		catch (UnsatisfiedLinkError e) {
			Log.e(TAG, "renderScreen not found");
		}

		// Detect if calculator is turned on by testing if a pixel (we'll just
		// use (0,0)) is black
		if (mBitmap.getPixel(0, 0) == Color.BLACK) {
			// In this case, draw the secret Android overlay
			drawOverlay(canvas);
			return;
		}

		// If we get here, then we know that one screen frame, at least,
		// is being drawn. So we can look at this to guess that the ROM is
		// running ok.
		mHasDrawnValidFrame = true;

		canvas.drawBitmap(mBitmap, null, mDstRect, null);

		// We'll refresh the screen as fast as possible
		postInvalidate();
	}

	/**
	 * Returns whether a valid frame has been drawn. We can use this to
	 * enable/disable keypresses and detect ROM crashes.
	 * 
	 * @return
	 */
	public boolean hasDrawnValidFrame() {
		return mHasDrawnValidFrame;
	}

	private void drawOverlay(Canvas canvas) {
		if (mOverlay == null) {
			mOverlay = BitmapFactory.decodeResource(getResources(), R.drawable.android_overlay);
			mOverlaySrcRect = new Rect(0, 0, mOverlay.getWidth(), mOverlay.getHeight());
			mOverlayDstRect = new Rect(mDstRect.right - mOverlaySrcRect.right,
					mDstRect.bottom - mOverlaySrcRect.bottom, mDstRect.right, mDstRect.bottom);
		}
		canvas.drawBitmap(mOverlay, mOverlaySrcRect, mOverlayDstRect, null);
	}
}
