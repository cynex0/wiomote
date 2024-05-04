package se.gu.wiomote.configurations;

import android.util.Log;

import androidx.annotation.NonNull;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class Configuration {
    private static final String TAG = "Configuration";
    private static final String UUID_KEY = "uuid";
    private static final String NAME_KEY = "name";
    private static final String COMMANDS_KEY = "commands";
    private static final String KEYCODE_KEY = "keyCode";
    private static final String COMMAND_KEY = "command";
    private final String uuid;
    private final Map<Integer, Command> commands;
    public String name;

    private Configuration(String uuid, @NonNull String name,
                          @NonNull Map<Integer, Command> commands) {
        this.uuid = uuid;
        this.name = name;
        this.commands = commands;
    }

    private Configuration(String uuid, @NonNull String name, @NonNull JSONArray commands) {
        this(uuid, name, toCommandMap(commands));
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

    public Command getCommandForKeyCode(int keyCode) {
        return commands.getOrDefault(keyCode, null);
    }

    public void addCommand(int keyCode, Command command) {
        commands.put(keyCode, command);
    }


    /* JSON Format (prettified):
    {
      "uuid" : "...",
      "name" : "...",
      "commands" : [
        {
          "keyCode" : <number>,
          "command" :
            {
              "label" : "...",
              "dataLength" : <length>,
              "rawData" : [ <byte0>, ...]
            }
        }
      ]
    }
     */
    public String serializeJSON() {
        StringBuilder builder = new StringBuilder();

        builder.append("{")
                .append("\"" + UUID_KEY + "\":\"").append(uuid).append("\",") // "uuid":"<uuid>",
                .append("\"" + NAME_KEY + "\":\"").append(uuid).append("\",") // "name":"<name>",
                .append("\"" + COMMANDS_KEY + "\":["); // "commands":[

        commands.forEach((keyCode, command) -> {
            builder.append("{")
                    .append("\"" + KEYCODE_KEY + "\":").append(keyCode).append(",") // "keyCode":<number>,
                    .append("\"" + COMMAND_KEY + "\":") // "command":
                    .append(command.serializeJSON()) // (see Command.java)
                    .append(",");
        });

        if (!commands.isEmpty()) builder.deleteCharAt(builder.length() - 1); // Delete trailing comma if at least 1 command is present
        builder.append("]}");

        Log.i(TAG, "serializeJSON: " + builder);
        return builder.toString();
    }

    public static Configuration deserializeJSON(String jsonString) {
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            return new Configuration(jsonObject.getString(UUID_KEY),
                    jsonObject.getString(NAME_KEY),
                    jsonObject.getJSONArray(COMMANDS_KEY));
        } catch (JSONException exception) {
            Log.e(TAG, "deserialize: Malformed configuration.");

            return null;
        }
    }

    private static Map<Integer, Command> toCommandMap(JSONArray array) {
        Map<Integer, Command> map = new HashMap<>();

        for(int index = 0; index < array.length(); index++) {
            try {
                JSONObject entry = array.getJSONObject(index);

                map.put(entry.getInt(KEYCODE_KEY),
                        Command.deserializeJSON((entry.getJSONObject(COMMAND_KEY))));
            } catch (Exception exception) {
                Log.e(TAG, "deserialize: Malformed command - skipping...");
            }
        }

        return map;
    }
}
