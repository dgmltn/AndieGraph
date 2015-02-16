package net.supware.tipro.model;

import java.io.File;

import android.util.Log;

public class TIGutsModel {
	private static final String TAG = TIGutsModel.class.getSimpleName();

	// These correspond with the constants defined in TI85.h
	public static final int ATI_TI85 = 0x0000;
	public static final int ATI_TI86 = 0x0100;
	public static final int ATI_TI82 = 0x0200;
	public static final int ATI_TI83 = 0x0400;
	public static final int ATI_TI83P = 0x0800;
	public static final int ATI_TI83SE = 0x1000;
	public static final int ATI_TI84 = 0x2000; // Not fully implemented in AlmostTI
	public static final int ATI_TI84P = 0x2000; // Not fully implemented in AlmostTI
	public static final int ATI_TI84SE = 0x4000; // Not fully implemented in AlmostTI

	public int mModelId;
	public String mRamFilename;
	public String mRomFilename;

	public TIGutsModel(int modelId, String ramFilename, String romFilename) {
		mModelId = modelId;
		mRamFilename = ramFilename;
		mRomFilename = romFilename;

		if (Log.isLoggable(TAG, Log.DEBUG))
			Log.d(TAG, "ROM = " + mRomFilename + ", RAM = " + mRamFilename);
	}

	/**
	 * Returns the (guessed) model of the ROM of the file passed in.
	 * 
	 * @param file
	 * @return one of the ATI_* constants defined in this module.
	 */
	public static int getRomFileType(File file) {
		String ucfilename = file.getName().toUpperCase();
		long size = file.length();

		if (!ucfilename.contains("TI"))
			return -1;

		if (size == 0x20000 && ucfilename.contains("82"))
			return ATI_TI82;

		if (size == 0x40000 && ucfilename.contains("83"))
			return ATI_TI83;

		if (size == 0x80000 && ucfilename.contains("83"))
			return ATI_TI83P;

		if (size == 0x20000 && ucfilename.contains("85"))
			return ATI_TI85;

		if (size == 0x40000 && ucfilename.contains("86"))
			return ATI_TI86;

		return -1;
	}
}
