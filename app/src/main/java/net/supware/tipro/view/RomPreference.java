package net.supware.tipro.view;

import android.content.Context;
import android.preference.PreferenceCategory;
import android.widget.Checkable;

import net.supware.tipro.R;

public class RomPreference extends RadioButtonPreference {
	public static final String TAG = RomPreference.class.getSimpleName();

	int mModelResId;
	Checkable mRadioView;

	public RomPreference(Context context, PreferenceCategory group, int modelResId, String filename) {
		super(context, group);
		mModelResId = modelResId;
		setWidgetLayoutResource(R.layout.preference_widget_radio);

		setTitle(context.getString(mModelResId));
		setSummary(filename);
		setKey("rom_filename");
		setValue(filename);
	}
}
