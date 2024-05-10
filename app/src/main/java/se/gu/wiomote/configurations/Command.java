package se.gu.wiomote.configurations;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;

public class Command {
    private static final String TAG = "Command";
    private static final String LABEL_KEY = "label";
    private static final String LENGTH_KEY = "dataLength";
    private static final String DATA_KEY = "rawData";
    private final int[] rawData;
    public String label;

    public Command(String label, int[] rawData) {
        this.label = label;
        this.rawData = rawData;
    }

    /* JSON Format (prettified):
    {
      "label" : "...",              // can be omitted when sending to the terminal
      "dataLength" : <length>,
      "rawData" : [ <byte0>, ...]
    }
    */
    public String serialize(boolean omitLabel) {
        StringBuilder builder = new StringBuilder();

        builder.append("{");

        if(!omitLabel) builder.append("\"" + LABEL_KEY + "\":\"").append(label).append("\","); // "label":"...",

        builder.append("\"" + LENGTH_KEY + "\":").append(rawData.length).append(",")       // "dataLength":<length>,
                .append("\"" + DATA_KEY + "\":")                                            // "bytes":[
                .append(Arrays.toString(rawData).replaceAll("\\s+", ""))   // <byte0>,...
                .append("}");

        return builder.toString();
    }

    public String serialize() {
        return serialize(false);
    }

    /* Expected format (may contain more fields):
    {
      "label" : "...",
      "rawData" : [ <byte0>, ...]
    }
    */
    public static Command deserialize(JSONObject jsonObject) throws JSONException {
        JSONArray bytesJSON = jsonObject.getJSONArray(DATA_KEY);
        int[] bytes = new int[bytesJSON.length()];

        for (int i = 0; i < bytesJSON.length(); i++) {
            bytes[i] = bytesJSON.getInt(i);
        }

        return new Command(jsonObject.getString(LABEL_KEY), bytes);
    }

    public static Command deserialize(String jsonString) {
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            return deserialize(jsonObject);
        } catch (JSONException exception) {
            Log.e(TAG, "deserialize: Malformed command.");

            return null;
        }
    }
}
