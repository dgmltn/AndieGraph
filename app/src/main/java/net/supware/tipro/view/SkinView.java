package net.supware.tipro.view;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import net.supware.tipro.model.TIButton;

public class SkinView extends View {
	private static final String TAG = SkinView.class.getSimpleName();

	Context mContext;
	ScreenView mScreenView;
	Paint mSkinPaint;
	Paint mButtonPaint;

	Bitmap mSkinBitmap;

	/**
	 * Actual height and width of the emulated screen (128x64?)
	 */
	int mPixelWidth, mPixelHeight;

	/**
	 * Bounds for the emulated screen pixels (relative to the skin definition).
	 */
	Rect mSkinPixelRegion;

	/**
	 * Bounds for emulated screen including trim, used for zoom.
	 * mSkinScreenRegion: relative to skin sizes. mViewScreenRegion: relative to
	 * this view's sizes.
	 */
	Rect mSkinScreenRegion;
	Rect mViewScreenRegion;

	/**
	 * Bounds for button region, used for zoom. mSkinButtonRegion: relative to
	 * skin sizes. mViewButtonRegion: relative to this view's sizes.
	 */
	Rect mSkinButtonRegion;
	Rect mViewButtonRegion;

	/**
	 * Used for drawing the skin onto the screen. They will be computed from
	 * m(Skin|View)(Screen|Button)Region and depending on mIsZoomed
	 */
	Rect mSkinScreenDrawableRegion, mViewScreenDrawableRegion;
	Rect mSkinButtonDrawableRegion, mViewButtonDrawableRegion;

	/**
	 * Whether to use mSkin*ZoomBounds or full bitmap when displaying the skin.
	 */
	boolean mIsZoomed = false;

	/**
	 * Region of skin that should be drawn on-screen. If mIsZoomed, it would be
	 * cropped to Union(mSkinButtonRegion, mSkinScreenRegion), if not, then the
	 * full bounds of mSkinBitmap.
	 */
	Rect mSkinVisibleRegion;

	/**
	 * Where on the view everything will be drawn (probably just the full view).
	 */
	Rect mViewRegion;

	/**
	 * Region to display pressed button (relative to screen)
	 */
	RectF mPressedButtonRectF;
	Rect mInvalidateRect;

	public SkinView(Context context, AttributeSet attributes) {
		super(context, attributes);
		mContext = context;
	}

	public void setSkinResource(int resId) {
		mSkinBitmap = BitmapFactory.decodeResource(mContext.getResources(),
				resId);
	}

	public void initScreen(ScreenView screenView, int width, int height) {
		mPixelWidth = width;
		mPixelHeight = height;
		mScreenView = screenView;
		mScreenView.setPixelSize(width, height);
	}

	public void setIsZoomed(boolean isZoomed) {
		mIsZoomed = isZoomed;
		requestLayout();
		invalidate();
	}

	public void initFromJSON(JSONObject jsSkin) throws JSONException {
		int left, right, top, bottom;

		JSONObject jsPixels = jsSkin.getJSONObject("screenPixels");
		left = jsPixels.getInt("left");
		top = jsPixels.getInt("top");
		right = jsPixels.getInt("right");
		bottom = jsPixels.getInt("bottom");
		mSkinPixelRegion = new Rect(left, top, right, bottom);

		JSONObject jsScreen = jsSkin.getJSONObject("screenRegion");
		left = jsScreen.getInt("left");
		top = jsScreen.getInt("top");
		right = jsScreen.getInt("right");
		bottom = jsScreen.getInt("bottom");
		mSkinScreenRegion = new Rect(left, top, right, bottom);

		JSONObject jsButtons = jsSkin.getJSONObject("buttonRegion");
		left = jsButtons.getInt("left");
		top = jsButtons.getInt("top");
		right = jsButtons.getInt("right");
		bottom = jsButtons.getInt("bottom");
		mSkinButtonRegion = new Rect(left, top, right, bottom);
	}

	@Override
	protected void onLayout(boolean changed, int left, int top, int right,
			int bottom) {
		super.onLayout(changed, left, top, right, bottom);

		if (mSkinBitmap == null)
			return;

		// Create a region representing where on the view everything will
		// be drawn.
		// mViewRegion = new Rect(0, 0, 320, 480);
		mViewRegion = new Rect(0, 0, getWidth(), getHeight());

		// Calculate the visible region. If zoomed, then crop as tightly as
		// possible to the Screen region and the Button region.
		if (mIsZoomed) {
			mSkinVisibleRegion = new Rect(mSkinButtonRegion);
			mSkinVisibleRegion.union(mSkinScreenRegion);
		} else {
			mSkinVisibleRegion = new Rect(0, 0, mSkinBitmap.getWidth(),
					mSkinBitmap.getHeight());
		}

		// Calculate the position for the buttons. This should start directly
		// below the screen.
		mViewButtonRegion = transformRect(mSkinButtonRegion,
				mSkinVisibleRegion, mViewRegion);

		// Calculate position for the emulated screen. We want the rendered
		// screen size to be an integer multiple of its pixel size
		// i.e. (128*m x 64*n)
		mViewScreenRegion = transformRect(mSkinScreenRegion,
				mSkinVisibleRegion, mViewRegion);
		Rect viewPixelsRegion = transformRect(mSkinPixelRegion,
				mSkinVisibleRegion, mViewRegion);
		viewPixelsRegion.inset(viewPixelsRegion.width() % mPixelWidth / 2, 0);
		viewPixelsRegion.right -= viewPixelsRegion.width() % mPixelWidth;

		// There's no need to waste vertical space above and below the 64*n
		// So shrink/expand the screen region and the button region
		// appropriately
		int dy = viewPixelsRegion.height() % mPixelHeight;
		dy = dy > mPixelHeight / 2 ? mPixelHeight - dy : -dy;
		mViewScreenRegion.bottom += dy;
		viewPixelsRegion.bottom += dy;
		mViewButtonRegion.top += dy;

		// Setup view parameters for emulated screen pixels
		mScreenView.setViewPixelsRegion(viewPixelsRegion);

		// Setup the rectangles to draw onto the screen
		mSkinScreenDrawableRegion = new Rect(mSkinVisibleRegion.left,
				mSkinVisibleRegion.top, mSkinVisibleRegion.right,
				mSkinScreenRegion.bottom);
		mViewScreenDrawableRegion = new Rect(mViewRegion.left, mViewRegion.top,
				mViewRegion.right, mViewScreenRegion.bottom);

		mSkinButtonDrawableRegion = new Rect(mSkinVisibleRegion.left,
				mSkinButtonRegion.top, mSkinVisibleRegion.right,
				mSkinVisibleRegion.bottom);
		mViewButtonDrawableRegion = new Rect(mViewRegion.left,
				mViewButtonRegion.top, mViewRegion.right, mViewRegion.bottom);

		invalidate();
	}

	@Override
	protected void onDraw(Canvas canvas) {
		// called when view is drawn

		if (mSkinBitmap == null)
			return;

		if (mSkinPaint == null) {
			mSkinPaint = new Paint();
			mSkinPaint.setFilterBitmap(true);
		}
		if (mButtonPaint == null) {
			mButtonPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
			mButtonPaint.setColor(0xD0FFFFFF);
		}

		// Draw the skin
		canvas.drawBitmap(mSkinBitmap, mSkinScreenDrawableRegion,
				mViewScreenDrawableRegion, mSkinPaint);
		canvas.drawBitmap(mSkinBitmap, mSkinButtonDrawableRegion,
				mViewButtonDrawableRegion, mSkinPaint);

		// If a button is pressed, draw that
		if (mPressedButtonRectF != null) {
			canvas.drawRoundRect(mPressedButtonRectF, 5, 5, mButtonPaint);
		}
	}

	public void setPressedButton(TIButton button) {
		mInvalidateRect = transformRect(button.rect, mSkinButtonRegion,
				mViewButtonRegion);
		mPressedButtonRectF = new RectF(mInvalidateRect);

		// Add a 2 pixel border, to be safe
		mInvalidateRect.inset(-2, -2);

		// Copy mInvalidateRect because .invalidate is destructive
		Rect dirty = new Rect(mInvalidateRect);
		this.invalidate(dirty);
	}

	public void clearPressedButton() {
		mPressedButtonRectF = null;

		if (mInvalidateRect != null) {

			// Copy mInvalidateRect because .invalidate is destructive
			Rect dirty = new Rect(mInvalidateRect);
			this.invalidate(dirty);

			mInvalidateRect = null;
		}
	}

	public Point screenToButton(int x, int y) {
		return transformPoint(x, y, mViewButtonRegion, mSkinButtonRegion);
	}

	private Point transformPoint(int x, int y, Rect from, Rect to) {

		// Normalize position of point p in from rect (0.0 -> 1.0)
		double dx = (double) (x - from.left) / from.width();
		double dy = (double) (y - from.top) / from.height();

		// Transpose this normalized number to the new rect
		int tox = (int) (dx * to.width()) + to.left;
		int toy = (int) (dy * to.height()) + to.top;

		return new Point(tox, toy);
	}

	private Rect transformRect(Rect r, Rect from, Rect to) {
		Point tl = transformPoint(r.left, r.top, from, to);
		Point br = transformPoint(r.right, r.bottom, from, to);
		return new Rect(tl.x, tl.y, br.x, br.y);
	}

}
