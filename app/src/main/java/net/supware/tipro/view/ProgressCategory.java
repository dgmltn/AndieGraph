package net.supware.tipro.view;

import android.content.Context;
import android.preference.PreferenceCategory;
import android.util.AttributeSet;
import android.view.View;

import net.supware.tipro.R;

public class ProgressCategory extends PreferenceCategory {

	private boolean mProgress = false;
	private View oldView = null;

	public ProgressCategory(Context context, AttributeSet attrs) {
		super(context, attrs);
		setLayoutResource(R.layout.preference_progress_category);
	}

	@Override
	public void onBindView(View view) {
		super.onBindView(view);
		View textView = view.findViewById(R.id.searching_text);
		View progressBar = view.findViewById(R.id.searching_progress);

		int visibility = mProgress ? View.VISIBLE : View.INVISIBLE;
		textView.setVisibility(visibility);
		progressBar.setVisibility(visibility);

		if (oldView != null) {
			oldView.findViewById(R.id.searching_progress).setVisibility(
					View.GONE);
			oldView.findViewById(R.id.searching_text).setVisibility(View.GONE);
			oldView.setVisibility(View.GONE);
		}
		oldView = view;
	}

	/**
	 * Turn on/off the progress indicator and text on the right.
	 * 
	 * @param progressOn
	 *            whether or not the progress should be displayed
	 */
	public void setProgress(boolean progressOn) {
		mProgress = progressOn;
		notifyChanged();
	}
}
