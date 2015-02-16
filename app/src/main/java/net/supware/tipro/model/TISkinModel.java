package net.supware.tipro.model;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.SortedSet;
import java.util.TreeSet;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Point;
import android.util.Log;

import net.supware.tipro.view.ScreenView;
import net.supware.tipro.view.SkinView;

public class TISkinModel {
	public static final String TAG = TISkinModel.class.getSimpleName();

	int mPixelWidth;
	int mPixelHeight;
	TreeSet<TIButton> buttons;
	int mSkinMetaResId;
	int mSkinDrawableResId;

	public TISkinModel(int pixelWidth, int pixelHeight, int skinMetaResId,
			int skinDrawableResId) {
		mPixelWidth = pixelWidth;
		mPixelHeight = pixelHeight;
		mSkinMetaResId = skinMetaResId;
		mSkinDrawableResId = skinDrawableResId;
	}

	// Read model info into JSONObject
	public void initFromRes(Context context, SkinView skinView,
			ScreenView screenView) {

		Resources resources = context.getResources();

		// Read JSON skin definition metadata
		try {
			JSONObject json = readJSONResource(resources, mSkinMetaResId);

			// Buttons
			JSONArray jsButtons = json.getJSONArray("buttons");

			buttons = new TreeSet<TIButton>();
			if (Log.isLoggable(TAG, Log.DEBUG)) {
				Log.d(TAG, "model has " + jsButtons.length() + " buttons");
			}

			for (int i = 0; i < jsButtons.length(); i++) {
				JSONObject jsButton = jsButtons.getJSONObject(i);
				buttons.add(new TIButton(jsButton));
			}

			skinView.initFromJSON(json);
			skinView.setSkinResource(mSkinDrawableResId);
			skinView.initScreen(screenView, mPixelWidth, mPixelHeight);
			skinView.requestLayout();

		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private JSONObject readJSONResource(Resources resources, int resId)
			throws JSONException, IOException {

		InputStream is = resources.openRawResource(resId);

		StringBuilder sb = new StringBuilder();
		BufferedReader br = new BufferedReader(new InputStreamReader(is), 8192);

		String line;
		while ((line = br.readLine()) != null) {
			sb.append(line);
		}

		String jsonString = sb.toString();
		JSONObject json = new JSONObject(jsonString);

		return json;
	}

	/**
	 * Determines what button is located at the given coordinates. May return
	 * null.
	 * 
	 * @param p
	 * @return TIButton object, or null
	 */
	public TIButton getButtonAt(Point p) {
		if (buttons == null)
			return null;

		// Create a temporary "button" for comparison purposes
		TIButton t = new TIButton(p);

		SortedSet<TIButton> possibilities = buttons.tailSet(t);

		for (TIButton button : possibilities)
			if (button.contains(p))
				return button;

		return null;
	}
}
