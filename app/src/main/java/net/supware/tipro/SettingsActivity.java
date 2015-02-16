package net.supware.tipro;

import java.io.File;
import java.util.HashSet;
import java.util.Iterator;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.text.TextUtils;

import net.supware.tipro.model.TIGutsModel;
import net.supware.tipro.view.ProgressCategory;
import net.supware.tipro.view.RomPreference;

public class SettingsActivity extends PreferenceActivity {
	private static final String TAG = SettingsActivity.class.getSimpleName();

	public static final String KEY_ROM_FILENAME = "rom_filename";
	public static final String KEY_FOUND_FILENAMES = "found_filenames";

	private ProgressCategory mRoms;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		addPreferencesFromResource(R.xml.preferences);

		mRoms = (ProgressCategory) findPreference("roms");
	}

	@Override
	protected void onResume() {
		super.onResume();
		new FindAllRomsTask().execute();
	}

	private class FindAllRomsTask extends FindRomsTask {
		HashSet<String> mFoundNames = new HashSet<String>();

		@Override
		public void onPreExecute() {
			String foundNamesCSV = getPreference(KEY_FOUND_FILENAMES, "");
			for (String n : foundNamesCSV.split(";")) {
				if (TextUtils.isEmpty(n))
					continue;

				addToFileQueue(n);
				addToFileQueue(new File(n).getParent());
			}

			mRoms.setProgress(true);
			mRoms.removeAll();
		}

		@Override
		public void onSearchDirectory(String dirname) {
			// Ignore this status in this task
		}

		@Override
		public void onFoundModel(int model, String filename) {
			if (mFoundNames.contains(filename))
				return;
			mFoundNames.add(filename);
			saveFoundNames();

			int modelResId = 0;
			switch (model) {
			case TIGutsModel.ATI_TI82:
				modelResId = R.string.model_ti82;
				break;
			case TIGutsModel.ATI_TI83:
				modelResId = R.string.model_ti83;
				break;
			case TIGutsModel.ATI_TI83P:
				modelResId = R.string.model_ti83p;
				break;
			case TIGutsModel.ATI_TI83SE:
				modelResId = R.string.model_ti83se;
				break;
			case TIGutsModel.ATI_TI85:
				modelResId = R.string.model_ti85;
				break;
			case TIGutsModel.ATI_TI86:
				modelResId = R.string.model_ti86;
				break;
			}
			RomPreference preference = new RomPreference(
					SettingsActivity.this, mRoms, modelResId, filename);
			mRoms.addPreference(preference);
		}

		@Override
		public void onPostExecute(Void result) {
			mRoms.setProgress(false);
		}
		
		private void saveFoundNames() {
			StringBuilder sb = new StringBuilder();
			Iterator<String> i = mFoundNames.iterator();
			while (i.hasNext()) {
				sb.append(i.next());
				sb.append(';');
			}
			if (sb.length() > 0)
				sb.setLength(sb.length() - 1);
			setPreference(KEY_FOUND_FILENAMES, sb.toString());
		}
	}

	private SharedPreferences mPreferences;

	public String getPreference(String key, String defaultValue) {
		if (mPreferences == null)
			mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		return mPreferences.getString(key, defaultValue);
	}

	public void setPreference(String key, String value) {
		if (mPreferences == null)
			mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		Editor editor = mPreferences.edit();
		editor.putString(key, value);
		editor.commit();
	}
}
