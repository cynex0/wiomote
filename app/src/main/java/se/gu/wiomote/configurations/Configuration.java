package se.gu.wiomote.configurations;

import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

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
    private final LinkedHashMap<Integer, Command> commands;
    public final String uuid;
    public String name;

    public Configuration(String uuid, String name,
                         @NonNull LinkedHashMap<Integer, Command> commands) {
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

    public boolean hasCommandForKeyCode(int keyCode) {
        return commands.containsKey(keyCode);
    }

    public Command getCommandForKeyCode(int keyCode) {
        return commands.get(keyCode);
    }

    public int getButtonCount() {
        return commands.size();
    }

    /**
     * Add command to command map from existing JSON String instance
     *
     * @param jsonString valid JSON object string representation
     * @return keyCode of the added command
     */
    public Integer addCommand(@NonNull String jsonString) {
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            Command command = Command.deserialize(jsonObject.getJSONObject(COMMAND_KEY));
            int keyCode = jsonObject.getInt(KEYCODE_KEY);

            commands.put(keyCode, command);

            return keyCode;
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

        builder.append("{")
                .append("\"" + KEYCODE_KEY + "\":").append(keyCode).append(",") // "keyCode":<keyCode>,
                .append("\"" + COMMAND_KEY + "\":"); // "command":

        if (command != null) builder.append(command.serialize(omitLabel)); // (see Command.java)
        builder.append("}");

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

    public List<Entry> getCustomCommands() {
        List<Entry> commands = new ArrayList<>();

        for (Map.Entry<Integer, Command> entry : this.commands.entrySet()) {
            if (entry.getKey() >= 0) {
                commands.add(new Entry(entry.getKey(), entry.getValue()));
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

    public static class Entry {
        public final int keyCode;
        public final Command command;

        public Entry(int keyCode, Command command) {
            this.keyCode = keyCode;
            this.command = command;
        }

        @Override
        public boolean equals(@Nullable Object object) {
            if(object == this) {
                return true;
            } else if (object instanceof Entry) {
                Entry entry = (Entry) object;

                return entry.keyCode == keyCode;
            }

            return false;
        }
    }
}
