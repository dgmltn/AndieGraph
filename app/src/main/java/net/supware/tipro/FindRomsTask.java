package net.supware.tipro;

import java.io.File;
import java.util.LinkedList;
import java.util.Queue;

import android.os.AsyncTask;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import net.supware.tipro.model.TIGutsModel;

abstract public class FindRomsTask extends
		AsyncTask<Void, FindRomsTask.ProgressData, Void> {
	
	private static final String TAG = FindRomsTask.class.getSimpleName();

	private String[] SEARCH_LOCATIONS = {
			Environment.getExternalStorageDirectory().getAbsolutePath(),
			"/mnt/sdcard", "/mnt/sdcard-ext" };

	protected Queue<String> mFileQueue = new LinkedList<>();

	class ProgressData {
		public int model;
		public String filename;

		public ProgressData(int m, String f) {
			this.model = m;
			this.filename = f;
		}
	}

	public void addToFileQueue(String... pathnames) {
		for (String path : pathnames) {
			if (TextUtils.isEmpty(path) || mFileQueue.contains(path))
				continue;
			
			mFileQueue.add(path);
			Log.e(TAG, "adding " + path);
		}
	}
	
	@Override
	public Void doInBackground(Void... params) {
		addToFileQueue(SEARCH_LOCATIONS);

		String next = mFileQueue.poll();
		while (next != null) {
			File nextFile = new File(next);
			if (nextFile.isDirectory()) {
				publishProgress(new ProgressData(-1, next));

				File subfiles[] = nextFile.listFiles();
				if (subfiles != null) {
					for (File f : subfiles) {
						mFileQueue.add(f.getAbsolutePath());
					}
				}
			}

			else if (nextFile.isFile()) {
				int model = TIGutsModel.getRomFileType(nextFile);
				if (model != -1)
					publishProgress(new ProgressData(model, next));
			}

			next = mFileQueue.poll();
		}
		return null;
	}

	@Override
	public void onProgressUpdate(ProgressData... progress) {
		for (ProgressData p : progress) {
			if (p.model == -1)
				onSearchDirectory(p.filename);
			else
				onFoundModel(p.model, p.filename);
		}
	}

	public abstract void onSearchDirectory(String dirname);

	public abstract void onFoundModel(int model, String filename);
}
