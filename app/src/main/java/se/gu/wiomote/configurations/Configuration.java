package se.gu.wiomote.configurations;

import android.util.Log;

import androidx.annotation.NonNull;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Configuration {
    private static final String TAG = "Configuration";
    private static final String KEYCODE_KEY = "keyCode";
    private static final String COMMAND_KEY = "command";
    private final Map<Integer, Command> commands;
    private final String uuid;
    public String name;

    public Configuration(String uuid, String name,
                         @NonNull Map<Integer, Command> commands) {
        this.uuid = uuid;
        this.name = name;
        this.commands = commands;
    }

    public Configuration(String uuid, String name) {
        this(uuid, name, new LinkedHashMap<>());
    }

    public Configuration(String uuid) {
        this(uuid, uuid);
    }

    public String getUUID() {
        return uuid;
    }

    /**
     * Add command to command map from existing JSON String instance
     *
     * @param jsonString valid JSON object string representation
     */
    public Command addCommand(@NonNull String jsonString) {
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            Command command = Command.deserialize(jsonObject.getJSONObject(COMMAND_KEY));

            commands.put(jsonObject.getInt(KEYCODE_KEY), command);

            return command;
        } catch (JSONException e) {
            Log.e(TAG, "deserialize: Malformed command.");

            return null;
        }
    }

    public void removeCommand(int keyCode) {
        commands.remove(keyCode);
    }

    /**
     * Deserialize a list of commands
     *
     * @param jsonString valid JSON object string representation
     */
    public static LinkedHashMap<Integer, Command> deserializeCommands(String jsonString) {
        try {
            JSONArray array = new JSONArray(jsonString);
            LinkedHashMap<Integer, Command> map = new LinkedHashMap<>();

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
        builder.append("[");

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

    public List<Command> getCustomCommands() {
        List<Command> commands = new ArrayList<>();

        for (Map.Entry<Integer, Command> entry : this.commands.entrySet()) {
            if (entry.getKey() >= 0) {
                commands.add(entry.getValue());
            }
        }

        return commands;
    }

    private boolean commandsEqual(Map<Integer, Command> otherCommands) {
        if (this.commands.size() != otherCommands.size()) return false;

        Iterator<Map.Entry<Integer, Command>> iterator = commands.entrySet().iterator();
        Iterator<Map.Entry<Integer, Command>> otherIterator = otherCommands.entrySet().iterator();

        while (iterator.hasNext()) {
            Map.Entry<Integer, Command> entry = iterator.next();
            Map.Entry<Integer, Command> otherEntry = otherIterator.next();

            if (!entry.equals(otherEntry)) return false;
        }
        return true;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null) return false;
        if (!(o instanceof Configuration)) return false;

        Configuration other = (Configuration) o;

        return commandsEqual(other.commands) ||
                name.equals(other.name);
    }
}
