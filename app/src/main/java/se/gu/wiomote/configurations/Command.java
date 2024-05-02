package se.gu.wiomote.configurations;

import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.Serializable;
import java.nio.charset.StandardCharsets;

public class Command implements Serializable {
    private static final String TAG = "Command";
    private static final String LABEL_KEY = "label";
    private static final String BYTES_KEY = "bytes";
    private final byte[] bytes;
    public String label;

    public Command(String label, byte[] bytes) {
        this.label = label;
        this.bytes = bytes;
    }

    public byte[] getBytes() {
        return bytes;
    }

    public String serializeJSON() {
        return "{" +
                "\"" + LABEL_KEY + "\":\"" + label + "\"," +
                "\"" + BYTES_KEY + "\":\"" + new String(bytes, StandardCharsets.UTF_8) + "\"" +
                "}";
    }

    public static Command deserializeJSON(JSONObject jsonObject) throws JSONException {
        return new Command(jsonObject.getString(LABEL_KEY),
                jsonObject.getString(BYTES_KEY).getBytes(StandardCharsets.UTF_8));
    }

    public static Command deserializeJSON(String jsonString) {
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            return deserializeJSON(jsonObject);
        } catch (JSONException exception) {
            Log.e(TAG, "deserialize: Malformed command.");

            return null;
        }
    }
}
