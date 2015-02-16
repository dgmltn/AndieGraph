package net.supware.tipro;

import java.io.File;

import android.content.Intent;
import android.text.Html;
import android.text.TextUtils;

import net.supware.tipro.model.TIGutsModel;
import net.supware.tipro.model.TISkinModel;

public class MainActivity extends Ti8xActivity {
	private static final String TAG = MainActivity.class.getSimpleName();

	private String mRomFilename;
	private boolean mHasSearchedSD = false;

	String mRomName;

	@Override
	public Intent getSettingsIntent() {
		return new Intent(this, SettingsActivity.class);
	}

	@Override
	public void onStart() {
		String romFilename = getPreference(
				SettingsActivity.KEY_ROM_FILENAME, "");

		// If ROM is gone...
		if (!TextUtils.isEmpty(romFilename)
				&& !(new File(romFilename)).exists()) {

			// Clear the ROM name from the settings
			setPreference(SettingsActivity.KEY_ROM_FILENAME, "");

			// Ax the RAM to go too
			File ramFile = new File(calculateRamFilename(romFilename));
			if (ramFile.exists())
				ramFile.delete();
			romFilename = "";
		}

		if (!romFilename.equals(mRomFilename)) {
			mHasSearchedSD = false;
			mSkinModel = null;
			mGutsModel = null;
			mRomFilename = romFilename;
		}

		super.onStart();
	}

	@Override
	public TISkinModel getSkinModel() {
		File romFile = new File(mRomFilename);
		int model = TIGutsModel.getRomFileType(romFile);

		switch (model) {
		case TIGutsModel.ATI_TI82:
			return new TISkinModel(96, 64, R.raw.ti82_480x800,
					R.drawable.ti82_480x800);
		case TIGutsModel.ATI_TI83:
			return new TISkinModel(96, 64, R.raw.ti83_480x800,
					R.drawable.ti83_480x800);
		case TIGutsModel.ATI_TI83P:
			return new TISkinModel(96, 64, R.raw.ti83p_480x800,
					R.drawable.ti83p_480x800);
		case TIGutsModel.ATI_TI85:
			return new TISkinModel(128, 64, R.raw.ti85_480x800,
					R.drawable.ti85_480x800);
		case TIGutsModel.ATI_TI86:
			return new TISkinModel(128, 64, R.raw.ti86_480x800,
					R.drawable.ti86_480x800);
		}

		// Default: just show SOMEthing
		return new TISkinModel(128, 64, R.raw.ti86_480x800,
				R.drawable.ti86_480x800);
	}

	@Override
	public TIGutsModel getGutsModel() {
		File romFile = new File(mRomFilename);
		int model = TIGutsModel.getRomFileType(romFile);
		if (model == -1) {
			if (!mHasSearchedSD) {
				// only search through SD automatically once per run
				mHasSearchedSD = true;
				new FindAnyRomTask().execute();
			}
			return null;
		}

		String ramFilename = calculateRamFilename(mRomFilename);

		return new TIGutsModel(model, ramFilename, mRomFilename);
	}

	private String calculateRamFilename(String romFilename) {
		int mid = romFilename.lastIndexOf(".");
		String ramFilename = (mid == -1 ? romFilename : romFilename.substring(
				0, mid)) + ".RAM";
		return ramFilename;
	}

	private class FindAnyRomTask extends FindRomsTask {

		@Override
		public void onPreExecute() {
			mHasSearchedSD = true;
			mSkinModel = null;
			mGutsModel = null;
			mRomName = getString(R.string.text_rom_name);
			showCrouton(getString(R.string.text_searching_for_x_rom, mRomName), null, true);
		}

		@Override
		public void onSearchDirectory(String dirname) {
			if (!isCancelled() && !isFinishing()) {
				String html = getString(R.string.text_searching_for_x_rom, mRomName);
				setCrouton(Html.fromHtml(html), dirname, true);
			}
		}

		@Override
		public void onFoundModel(int model, String filename) {
			setPreference(SettingsActivity.KEY_ROM_FILENAME, filename);
			setPreference(SettingsActivity.KEY_FOUND_FILENAMES, filename);

			cancel(true);
			hideCrouton();
			mRomFilename = filename;
			initSkin();
			startEmulator();
		}

		@Override
		public void onPostExecute(Void result) {
			if (!isCancelled() && !isFinishing()) {
				hideCrouton();
				String html = getString(R.string.crouton_rom_not_found);
				showCrouton(Html.fromHtml(html), null, false);
			}
		}
	}

}