package se.gu.wiomote.configurations;

import android.util.Log;

import androidx.annotation.NonNull;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;

public class Configuration {
    private static final String TAG = "Configuration";
    private static final String COMMANDS_KEY = "commands";
    private static final String KEYCODE_KEY = "keyCode";
    private static final String COMMAND_KEY = "command";
    private final String uuid;
    private final Map<Integer, Command> commands;
    public String name;

    public Configuration(String uuid, String name,
                          @NonNull Map<Integer, Command> commands) {
        this.uuid = uuid;
        this.name = name;
        this.commands = commands;
    }

    public Configuration(String uuid, String name) {
        this(uuid, name, new HashMap<>());
    }

    public Configuration(String uuid) {
        this(uuid, uuid);
    }

    public String getUUID() {
        return uuid;
    }

    /**
     * Add command to command map from existing {@link Command} instance
     */
    public void addCommand(int keyCode, Command command) {
        commands.put(keyCode, command);
    }

    /**
     * Add command to command map from existing JSON String instance
     *
     * @param jsonString valid JSON object string representation
     */
    public void addCommand(@NonNull String jsonString) {
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            addCommand(jsonObject.getInt(KEYCODE_KEY),
                    Command.deserialize(jsonObject.getJSONObject(COMMAND_KEY)));
        } catch (JSONException e) {
            Log.e(TAG, "deserialize: Malformed command.");
        }
    }

    /**
     * Deserialize a list of commands
     *
     * @param jsonString valid JSON object string representation
     */
    public static Map<Integer, Command> deserializeCommands(String jsonString) {
        try {
            JSONArray array = new JSONArray(jsonString);
            Map<Integer, Command> map = new HashMap<>();

            for (int index = 0; index < array.length(); index++) {
                try {
                    JSONObject entry = array.getJSONObject(index);

                    map.put(entry.getInt(KEYCODE_KEY),
                            Command.deserialize((entry.getJSONObject(COMMAND_KEY))));
                } catch (Exception exception) {
                    Log.e(TAG, "deserialize: Malformed command - skipping...");
                }
            }

            return map;
        } catch (JSONException exception) {
            Log.e(TAG, "deserialize: Malformed configuration.");
        }

        return null;
    }

    /*
    {
        "keyCode": <keyCode>,
        "command": {
            "label" : "...",              // can be omitted when sending to the terminal
            "dataLength" : <length>,
            "rawData" : [ <byte0>, ...]
        }
    }
    */
    public String serializeCommand(int keyCode, boolean omitLabel) {
        StringBuilder builder = new StringBuilder();
        Command command = commands.getOrDefault(keyCode, null);

        if (command == null) {
            return null;
        }

        builder.append("{")
                .append("\"" + KEYCODE_KEY + "\":").append(keyCode).append(",") // "keyCode":<keyCode>,
                .append("\"" + COMMAND_KEY + "\":") // "command":
                .append(command.serialize(omitLabel)) // (see Command.java)
                .append("}");

        return builder.toString();
    }

    public String serializeConfig() {
        return serializeConfig(false);
    }

    public String serializeConfig(boolean omitLabels) {
        StringBuilder builder = new StringBuilder();
        builder.append("\"" + COMMANDS_KEY + "\":["); // "commands":[

        boolean hasAtLeastOneItem = false;

        for (Map.Entry<Integer, Command> entry : commands.entrySet()) {
            Integer keyCode = entry.getKey();

            String serialCommand = serializeCommand(keyCode, omitLabels);

            if (serialCommand != null) {
                if (hasAtLeastOneItem) {
                    builder.append(",");
                }

                builder.append(serialCommand);

                hasAtLeastOneItem = true;
            }
        }

        builder.append("]");

        return builder.toString();
    }
}
