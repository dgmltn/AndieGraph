package net.supware.tipro;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

import java.util.ArrayList;
import java.util.HashMap;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Point;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.Display;
import android.view.HapticFeedbackConstants;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import net.supware.tipro.model.TIButton;
import net.supware.tipro.model.TIGutsModel;
import net.supware.tipro.model.TISkinModel;
import net.supware.tipro.view.ScreenView;
import net.supware.tipro.view.SkinView;


public abstract class Ti8xActivity extends FullScreenActivity {
	private static final String TAG = Ti8xActivity.class.getSimpleName();

	private static final String KEY_HAPTIC_FEEDBACK = "haptic_feedback";
	private static final String KEY_ZOOM = "zoom";
	private static final String KEY_WAKE_LOCK = "wake_lock";

	private static final int IO_BUFFER_SIZE = 4 * 1024;

	protected static final int DIALOG_ABOUT = 2;
	protected static final int DIALOG_SUPPORT = 4;

	public SkinView mSkinView;
	public ScreenView mScreenView;
	public Thread mThread;
	public TISkinModel mSkinModel;
	public TIGutsModel mGutsModel;
	public HashMap<Integer, TIButton> mPressedButtons = new HashMap<Integer, TIButton>();

	public PowerManager.WakeLock mWakeLock;

	// Settings
	private SharedPreferences mPreferences;
	public boolean mDoVibrate = true;
	public boolean mIsZoomed = false;
	public boolean mDoWakeLock = false;

	private boolean isPausing = false;
	private Runnable mEmulatorRunnable = new Runnable() {
		public void run() {
			if (mWakeLock != null && mDoWakeLock) {
				mWakeLock.acquire();
			}

			NativeLib.start(mGutsModel.mModelId, mGutsModel.mRomFilename,
					mGutsModel.mRamFilename);

			if (mWakeLock != null && mWakeLock.isHeld()) {
				mWakeLock.release();
			}

			if (!isPausing) {
				finish();
			}
		}
	};

	private OnTouchListener mSkinOnTouchListener = new OnTouchListener() {

		@Override
		public boolean onTouch(View v, MotionEvent event) {
			if (mSkinModel == null) {
				return false;
			}

			// Don't allow keypresses until we know something is happening on
			// the screen (i.e. the ROM is active).
			if (mScreenView == null || !mScreenView.hasDrawnValidFrame()) {
				return false;
			}

			// Ensuring that we track each finger for multitouch
			int pointerIndex = event.getActionIndex();
			int pointerId = event.getPointerId(pointerIndex);

			Point p = mSkinView.screenToButton((int) event.getX(pointerIndex),
				(int) event.getY(pointerIndex));

			if (Log.isLoggable(TAG, Log.DEBUG)) {
				Log.d(TAG, "skin onTouch event at adjusted (" + p.x + ", "
					+ p.y + ")");
			}

			switch (event.getActionMasked()) {

			case MotionEvent.ACTION_UP:
			case MotionEvent.ACTION_POINTER_UP:
				// We know to un-press the currently pressed button
				if (mPressedButtons.containsKey(pointerId)) {
					keyUp(pointerId);
				}
				break;

			case MotionEvent.ACTION_DOWN:
			case MotionEvent.ACTION_POINTER_DOWN:

				TIButton currentButton = mSkinModel.getButtonAt(p);
				// Check if clicking or moving onto a new button
				if (!mPressedButtons.containsKey(pointerId)
					&& (currentButton != null)) {
					mPressedButtons.put(pointerId, currentButton);
					keyDown(pointerId);
				}

				break;

			case MotionEvent.ACTION_MOVE:
				int pointerCount = event.getPointerCount();
				HashMap<Integer, TIButton> currentButtons = new HashMap<Integer, TIButton>();
				for(int i = 0; i < pointerCount; ++i)
				{
					pointerIndex = i;
					pointerId = event.getPointerId(pointerIndex);
					p = mSkinView.screenToButton((int) event.getX(pointerIndex),
												 (int) event.getY(pointerIndex));
					currentButtons.put(pointerId, mSkinModel.getButtonAt(p));
				}
				ArrayList<Integer> noLongerHeldButtons = new ArrayList<Integer>();
				for (HashMap.Entry<Integer, TIButton> entry : mPressedButtons.entrySet()) {
					if (entry.getValue() != null & !currentButtons.containsValue(entry.getValue()))
					{
						noLongerHeldButtons.add(entry.getKey());
					}
				}
				for (int i = 0; i < noLongerHeldButtons.size() ; i++) {
					keyUp(noLongerHeldButtons.get(i));
				}
				for (HashMap.Entry<Integer, TIButton> entry : currentButtons.entrySet()) {
					if (entry.getValue() != null & !mPressedButtons.containsValue(entry.getValue()))
					{
						mPressedButtons.put(entry.getKey(), entry.getValue());
						keyDown(entry.getKey());
					}
				}

				break;

			}

			return false;
		}

		private void keyUp(int pointerId) {
			if (mThread == null || !mThread.isAlive()) {
				return;
			}

			mScreenView.mKeyQueue.add(new ScreenView.KeyState(mPressedButtons.get(pointerId).keycode, false));
			mSkinView.clearPressedButton(mPressedButtons.get(pointerId));
			mPressedButtons.remove(pointerId);
		}

		private void keyDown(int pointerId) {
			if (mThread == null || !mThread.isAlive()) {
				return;
			}

			mScreenView.mKeyQueue.add(new ScreenView.KeyState(mPressedButtons.get(pointerId).keycode, true));

			mSkinView.setPressedButton(mPressedButtons.get(pointerId));

			// Vibrate
			if (mDoVibrate) {
				mSkinView.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			}
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "Andie Graph Wake Lock");

		mSkinView = (SkinView) findViewById(R.id.skin);
		mSkinView.setClickable(true);
		mSkinView.setEnabled(true);
		mSkinView.setIsZoomed(mIsZoomed);
		mSkinView.setOnTouchListener(mSkinOnTouchListener);

		mScreenView = (ScreenView) findViewById(R.id.screen);
	}

	@Override
	protected void onStart() {
		if (Log.isLoggable(TAG, Log.DEBUG)) {
			Log.d(TAG, "onStart()");
		}
		isPausing = false;

		mDoVibrate = getPreference(KEY_HAPTIC_FEEDBACK, true);
		mIsZoomed = getPreference(KEY_ZOOM, false);
		mDoWakeLock = getPreference(KEY_WAKE_LOCK, false);
		mSkinView.setIsZoomed(mIsZoomed);

		initSkin();

		NativeLib.onResume();
		startEmulator();

		super.onStart();
	}

	@Override
	protected void onStop() {
		if (Log.isLoggable(TAG, Log.DEBUG)) {
			Log.d(TAG, "onStop()");
		}
		isPausing = true;

		//NativeLib.stop();
		NativeLib.onPause();

		mScreenView.clear();

		super.onPause();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.menu, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int id = item.getItemId();

		if (id == R.id.menu_settings) {
			startActivity(getSettingsIntent());
			return true;
		}
		else if (id == R.id.menu_about) {
			showDialog(DIALOG_ABOUT);
			return true;
		}

		return false;
	}

	/**
	 * This can be overridden by an extending class.
	 *
	 * @return an intent starting this app's settings activity.
	 */
	public Intent getSettingsIntent() {
		return new Intent(this, SettingsActivity.class);
	}

	/**
	 * This should return a TISkinModel for the particular model of calculator.
	 *
	 * @return
	 */
	public abstract TISkinModel getSkinModel();

	/**
	 * This should return a TIGutsModel for the particular model of calculator.
	 *
	 * @return
	 */
	public abstract TIGutsModel getGutsModel();

	@Override
	protected Dialog onCreateDialog(int id) {
		AlertDialog.Builder builder;

		switch (id) {

		case DIALOG_ABOUT: {
			builder = new AlertDialog.Builder(this);
			builder.setIcon(getLauncherIconResId());
			builder.setTitle(R.string.app_name);
			builder.setCancelable(true);

			View view = View.inflate(this, R.layout.about_dialog_view, null);
			((TextView) view).setText(getString(R.string.dialog_about,
				getAppVersion()));
			builder.setView(view);

			builder.setPositiveButton(android.R.string.ok,
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) {
						dialog.dismiss();
					}
				});

			builder.setNeutralButton(R.string.button_support,
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) {
						showDialog(DIALOG_SUPPORT);
						dialog.dismiss();
					}
				});

			builder.setNegativeButton(R.string.button_donate,
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) {
						openDonateActivity();
						dialog.dismiss();
					}
				});

			return builder.create();
		}

		case DIALOG_SUPPORT: {
			builder = new AlertDialog.Builder(this);
			builder.setIcon(getLauncherIconResId());
			builder.setTitle(R.string.app_name);
			builder.setCancelable(true);

			View view = View.inflate(this,
				R.layout.support_request_dialog_view, null);
			builder.setView(view);

			TextView introText = (TextView) view.findViewById(R.id.support_request_intro);
			introText.setMovementMethod(LinkMovementMethod.getInstance());

			final EditText questionEdit = (EditText) view
				.findViewById(R.id.support_request_question);

			builder.setPositiveButton(android.R.string.ok,
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) {
						String question = questionEdit.getText().toString();
						if (TextUtils.isEmpty(question)) {
							return;
						}
						sendSupportEmail(question);
						dialog.dismiss();
					}
				});

			builder.setNegativeButton(android.R.string.cancel,
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id) {
						dialog.dismiss();
					}
				});

			return builder.create();
		}
		}

		return super.onCreateDialog(id);
	}

	public void initSkin() {
		if (mSkinModel == null) {
			mSkinModel = getSkinModel();
		}

		mSkinModel.initFromRes(this, mSkinView, mScreenView);
	}

	public boolean startEmulator() {
		if (mGutsModel == null) {
			mGutsModel = getGutsModel();
		}

		if (mGutsModel == null) {
			return false;
		}

		if (mThread != null && mThread.isAlive()) {
			return false;
		}

		mScreenView.setVisibility(View.VISIBLE);

		mThread = new Thread(mEmulatorRunnable);
		mThread.start();

		return true;
	}

	public File copyAssetToCache(String filename) {
		File dest = new File(getCacheDir(), filename);
		dest.deleteOnExit();
		if (!dest.exists()) {
			try {
				InputStream in = getAssets().open(filename);
				OutputStream out = new FileOutputStream(dest);
				byte[] b = new byte[IO_BUFFER_SIZE];
				int read;
				while ((read = in.read(b)) != -1) {
					out.write(b, 0, read);
				}
				in.close();
				out.close();
			}
			catch (Exception e) {
				e.printStackTrace();
				return null;
			}
		}
		return dest;
	}

	public String getPreference(String key, String defaultValue) {
		if (mPreferences == null) {
			mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		}
		return mPreferences.getString(key, defaultValue);
	}

	public boolean getPreference(String key, boolean defaultValue) {
		if (mPreferences == null) {
			mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		}
		return mPreferences.getBoolean(key, defaultValue);
	}

	public void setPreference(String key, String value) {
		if (mPreferences == null) {
			mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		}
		Editor editor = mPreferences.edit();
		editor.putString(key, value);
		editor.commit();
	}

	public void showCrouton(CharSequence primaryText, CharSequence secondaryText, boolean spinner) {
		setCrouton(primaryText, secondaryText, spinner);
		View crouton = findViewById(R.id.crouton);
		crouton.setAlpha(0f);
		crouton.setVisibility(View.VISIBLE);
		crouton.animate().alpha(1f).start();
	}

	public void setCrouton(CharSequence primaryText, CharSequence secondaryText, boolean spinner) {
		TextView primaryTextView = (TextView) findViewById(R.id.primary_text);
		if (primaryText == null) {
			primaryTextView.setVisibility(View.GONE);
		}
		else {
			primaryTextView.setVisibility(View.VISIBLE);
			primaryTextView.setText(primaryText);
			primaryTextView.setMovementMethod(LinkMovementMethod.getInstance());
		}

		TextView secondaryTextView = (TextView) findViewById(R.id.secondary_text);
		if (secondaryText == null) {
			secondaryTextView.setVisibility(View.GONE);
		}
		else {
			secondaryTextView.setVisibility(View.VISIBLE);
			secondaryTextView.setText(secondaryText);
			secondaryTextView.setMovementMethod(LinkMovementMethod.getInstance());
		}

		View progress = findViewById(R.id.progress);
		progress.setVisibility(spinner ? View.VISIBLE : View.GONE);
	}

	public void hideCrouton() {
		final View crouton = findViewById(R.id.crouton);
		crouton.animate().alpha(0f).withEndAction(new Runnable() {
			@Override
			public void run() {
				crouton.setVisibility(View.INVISIBLE);
			}
		}).start();
	}

	private void sendSupportEmail(String question) {
		Intent supportIntent = new Intent(Intent.ACTION_SEND);

		StringBuilder builder = new StringBuilder();
		builder.append(question + "\n\n");

		builder.append("App Version: " + getAppVersion() + "\n");
		builder.append("Model: " + Build.MODEL + "\n");

		Display display = getWindowManager().getDefaultDisplay();
		builder.append("Screen Size: " + display.getWidth() + "x"
			+ display.getHeight() + "\n");

		builder.append("SDK Version: " + Build.VERSION.SDK + "\n\n");

		supportIntent.putExtra(Intent.EXTRA_EMAIL,
			new String[] { "support@supware.net" });
		supportIntent.putExtra(Intent.EXTRA_SUBJECT,
			getString(R.string.text_support_request)
				+ getString(R.string.app_name));
		supportIntent.putExtra(Intent.EXTRA_TEXT, builder.toString());
		supportIntent.setType("message/rfc822");
		supportIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		try {
			startActivity(supportIntent);
		}
		catch (ActivityNotFoundException e) {
			Toast.makeText(this, R.string.toast_cannot_send_email,
				Toast.LENGTH_LONG);
		}
	}

	private void openDonateActivity() {
		Intent donateIntent = new Intent(Intent.ACTION_VIEW,
			Uri.parse("http://supware.net/donate"));
		try {
			startActivity(donateIntent);
		}
		catch (ActivityNotFoundException e) {
			Toast.makeText(this, R.string.toast_cannot_open_url,
				Toast.LENGTH_LONG);
		}
	}

	private PackageInfo getPackageInfo() {
		try {
			PackageInfo pkg = getPackageManager().getPackageInfo(
				getPackageName(), 0);
			return pkg;
		}
		catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		return null;
	}

	private String getAppVersion() {
		PackageInfo pkg = getPackageInfo();

		String version = pkg == null ? null : pkg.versionName;

		if (TextUtils.isEmpty(version)) {
			version = getString(R.string.text_unknown);
		}

		return version;
	}

	private int getLauncherIconResId() {
		PackageInfo pkg = getPackageInfo();
		return pkg == null ? 0 : pkg.applicationInfo.icon;
	}
}
