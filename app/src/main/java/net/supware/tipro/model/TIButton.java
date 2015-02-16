package net.supware.tipro.model;

import java.util.HashMap;

import org.json.JSONObject;

import android.graphics.Point;
import android.graphics.Rect;

public class TIButton implements Comparable<TIButton> {
	public static final int KBD_2ND = 0x00;
	public static final int KBD_ALPHA = 0x01;
	public static final int KBD_XVAR = 0x02;
	public static final int KBD_GRAPH = 0x03;
	public static final int KBD_MATRX = 0x04; /* TI83 key */
	public static final int KBD_STAT = 0x04; /* TI85 key */
	public static final int KBD_TABLE = 0x04; /* TI86 key */
	public static final int KBD_PRGM = 0x05;
	public static final int KBD_CUSTOM = 0x06;
	public static final int KBD_LOG = 0x07;
	public static final int KBD_DEL = 0x08;
	public static final int KBD_MORE = 0x09;
	public static final int KBD_SIN = 0x0A;
	public static final int KBD_COS = 0x0B;
	public static final int KBD_CLEAR = 0x0C;
	public static final int KBD_ENTER = 0x0D;
	public static final int KBD_TAN = 0x0E;
	public static final int KBD_LN = 0x0F;
	public static final int KBD_EE = 0x10;
	public static final int KBD_SQR = 0x11;
	public static final int KBD_STO = 0x12;
	public static final int KBD_ON = 0x13;
	public static final int KBD_SIGN = 0x14;
	public static final int KBD_F1 = 0x15;
	public static final int KBD_F2 = 0x16;
	public static final int KBD_F3 = 0x17;
	public static final int KBD_F4 = 0x18;
	public static final int KBD_F5 = 0x19;
	public static final int KBD_MODE = 0x1B; /* TI83 key */
	public static final int KBD_EXIT = 0x1B; /* TI85/86 key */
	public static final int KBD_LEFT = 0x1C;
	public static final int KBD_RIGHT = 0x1D;
	public static final int KBD_UP = 0x1E;
	public static final int KBD_DOWN = 0x1F;
	public static final int KBD_POWER = '^';
	public static final int KBD_LPARENT = '(';
	public static final int KBD_RPARENT = ')';
	public static final int KBD_DIV = '/';
	public static final int KBD_7 = '7';
	public static final int KBD_8 = '8';
	public static final int KBD_9 = '9';
	public static final int KBD_MUL = '*';
	public static final int KBD_COMMA = ',';
	public static final int KBD_4 = '4';
	public static final int KBD_5 = '5';
	public static final int KBD_6 = '6';
	public static final int KBD_MINUS = '-';
	public static final int KBD_2 = '2';
	public static final int KBD_3 = '3';
	public static final int KBD_PLUS = '+';
	public static final int KBD_1 = '1';
	public static final int KBD_0 = '0';
	public static final int KBD_DOT = '.';

	@SuppressWarnings("serial")
	private static final HashMap<String, Integer> KEYCODE_FOR = new HashMap<String, Integer>() {
		{
			put("KBD_2ND", KBD_2ND);
			put("KBD_ALPHA", KBD_ALPHA);
			put("KBD_XVAR", KBD_XVAR);
			put("KBD_GRAPH", KBD_GRAPH);
			put("KBD_MATRX", KBD_MATRX);
			put("KBD_STAT", KBD_STAT);
			put("KBD_TABLE", KBD_TABLE);
			put("KBD_PRGM", KBD_PRGM);
			put("KBD_CUSTOM", KBD_CUSTOM);
			put("KBD_LOG", KBD_LOG);
			put("KBD_DEL", KBD_DEL);
			put("KBD_MORE", KBD_MORE);
			put("KBD_SIN", KBD_SIN);
			put("KBD_COS", KBD_COS);
			put("KBD_CLEAR", KBD_CLEAR);
			put("KBD_ENTER", KBD_ENTER);
			put("KBD_TAN", KBD_TAN);
			put("KBD_LN", KBD_LN);
			put("KBD_EE", KBD_EE);
			put("KBD_SQR", KBD_SQR);
			put("KBD_STO", KBD_STO);
			put("KBD_ON", KBD_ON);
			put("KBD_SIGN", KBD_SIGN);
			put("KBD_F1", KBD_F1);
			put("KBD_F2", KBD_F2);
			put("KBD_F3", KBD_F3);
			put("KBD_F4", KBD_F4);
			put("KBD_F5", KBD_F5);
			put("KBD_MODE", KBD_MODE);
			put("KBD_EXIT", KBD_EXIT);
			put("KBD_LEFT", KBD_LEFT);
			put("KBD_RIGHT", KBD_RIGHT);
			put("KBD_UP", KBD_UP);
			put("KBD_DOWN", KBD_DOWN);
			put("KBD_POWER", KBD_POWER);
			put("KBD_LPARENT", KBD_LPARENT);
			put("KBD_RPARENT", KBD_RPARENT);
			put("KBD_DIV", KBD_DIV);
			put("KBD_7", KBD_7);
			put("KBD_8", KBD_8);
			put("KBD_9", KBD_9);
			put("KBD_MUL", KBD_MUL);
			put("KBD_COMMA", KBD_COMMA);
			put("KBD_4", KBD_4);
			put("KBD_5", KBD_5);
			put("KBD_6", KBD_6);
			put("KBD_MINUS", KBD_MINUS);
			put("KBD_2", KBD_2);
			put("KBD_3", KBD_3);
			put("KBD_PLUS", KBD_PLUS);
			put("KBD_1", KBD_1);
			put("KBD_0", KBD_0);
			put("KBD_DOT", KBD_DOT);
		}
	};

	public int keycode;
	public int x, y, w, h;
	public Rect rect;

	/*
	 * public TIButton(int keycode, int x, int y, int w, int h, int padding) {
	 * this.keycode = keycode; this.x = x - padding; this.y = y - padding;
	 * this.w = w + 2 * padding; this.h = h + 2 * padding; this.rect = new
	 * Rect(this.x, this.y, this.x + this.w, this.y + this.h); }
	 */

	public TIButton(JSONObject json) throws Exception {

		if (!json.has("padding") || !json.has("x") || !json.has("y")
				|| !json.has("w") || !json.has("h") || !json.has("key"))
			throw new Exception("invalid skin definition");

		int padding = json.getInt("padding");

		this.keycode = KEYCODE_FOR.get(json.getString("key"));
		this.x = json.getInt("x") - padding;
		this.y = json.getInt("y") - padding;
		this.w = json.getInt("w") + 2 * padding;
		this.h = json.getInt("h") + 2 * padding;

		this.rect = new Rect(this.x, this.y, this.x + this.w, this.y + this.h);
	}
	
	public TIButton(Point p) {
		this.x = p.x;
		this.y = p.y;
		this.w = 0;
		this.h = 0;
		this.rect = null;
	}

	@Override
	public int compareTo(TIButton that) {
		if (this.y < that.y)
			return 1;
		else if (this.y > that.y)
			return -1;
		else if (this.x < that.x)
			return 1;
		else if (this.x > that.x)
			return -1;

		return 0;
	}
	
	public boolean contains(Point p) {
		return this.x <= p.x && this.y <= p.y && this.rect.right > p.x && this.rect.bottom > p.y;
	}
}
