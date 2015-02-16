package net.supware.tipro.view;

import android.content.Context;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.view.View;
import android.widget.Checkable;

import net.supware.tipro.R;

public class RadioButtonPreference extends Preference {
	public static final String TAG = RadioButtonPreference.class
			.getSimpleName();

	private Checkable mRadioView;
	private String mValue;
	private PreferenceCategory mCategory;

	public RadioButtonPreference(Context context, PreferenceCategory group) {
		super(context);

		mCategory = (PreferenceCategory) group;

		setWidgetLayoutResource(R.layout.preference_widget_radio);
	}

	@Override
	protected void onBindView(View view) {
		View v = view.findViewById(R.id.radio);
		if (v == null || !(v instanceof Checkable))
			return;

		mRadioView = (Checkable) v;

		this.setChecked(getPersistedString("").equals(mValue));

		super.onBindView(view);
	}

	@Override
	protected void onClick() {
		this.setChecked(true);
		super.onClick();
	}

	public void setChecked(boolean isChecked) {
		if (mRadioView == null || mRadioView.isChecked() == isChecked)
			return;

		mRadioView.setChecked(isChecked);

		// When unchecking, just clear the radio button and be done.
		if (!isChecked)
			return;
		
		persistString(mValue);

		// uncheck others in the same group as this one
		if (mCategory == null)
			return;

		for (int i = mCategory.getPreferenceCount() - 1; i >= 0; i--) {
			Preference p = mCategory.getPreference(i);
			if (p != this) {
				if (p instanceof RadioButtonPreference) {
					RadioButtonPreference rbp = (RadioButtonPreference) p;
					rbp.setChecked(false);
				}
			}
		}
	}

	public String getValue() {
		return mValue;
	}

	public void setValue(String value) {
		this.mValue = value;
	}

}
