package se.gu.wiomote.application.activities.remote;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import se.gu.wiomote.R;
import se.gu.wiomote.application.DatabaseAccessActivity;
import se.gu.wiomote.application.activities.list.ConfigurationList;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;
import se.gu.wiomote.network.mqtt.WioMQTTClient;
import se.gu.wiomote.utils.Dialogs;

public class Remote extends DatabaseAccessActivity {
    static final String IR_SEND_TOPIC = "wiomote/ir/app";
    private static final String SHARED_PREFERENCES = "RemotePreference";
    private static final String UUID = "UUID";
    private static final String TYPE = "TYPE";
    private static final String TAG = "se.gu.wiomote.remote";
    private static ConfigurationType type = ConfigurationType.TV;
    private static String uuid = "a76953c5-0f00-4256-970f-cdd509a79224";
    private static boolean update = false;
    private final Map<Integer, View> basicButtonMap = new HashMap<>();
    private SharedPreferences preferences;
    private RecyclerView recyclerView;
    private TextView label;
    private Configuration config;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.remote);

        preferences = getSharedPreferences(SHARED_PREFERENCES, Context.MODE_PRIVATE);

        type = ConfigurationType.valueOf(preferences.getString(TYPE, ConfigurationType.TV.toString()));
        uuid = preferences.getString(UUID, "a76953c5-0f00-4256-970f-cdd509a79224");

        recyclerView = findViewById(R.id.recycler);
        label = findViewById(R.id.label);
        TextView configurations = findViewById(R.id.configurations);
        View power = findViewById(R.id.power);
        View ok = findViewById(R.id.ok);
        View up = findViewById(R.id.up);
        View left = findViewById(R.id.left);
        View down = findViewById(R.id.down);
        View right = findViewById(R.id.right);

        configurations.setOnClickListener(v -> startActivity(new Intent(Remote.this, ConfigurationList.class)));

        basicButtonMap.put(-1, power);
        basicButtonMap.put(-2, up);
        basicButtonMap.put(-3, right);
        basicButtonMap.put(-4, down);
        basicButtonMap.put(-5, left);
        basicButtonMap.put(-6, ok);

        updateLayout();
    }

    private void updateLayout() {
        String name;
        Cursor cursor = getDatabase().get(type, uuid);

        if (cursor.getCount() > 0) { // Configuration for current uuid exists in the db
            name = Database.getColumn(cursor, Database.Columns.NAME);
            Map<Integer, Command> commands = Configuration.deserializeCommands(
                    Database.getColumn(cursor, Database.Columns.COMMANDS)
            );

            cursor.close();

            if (uuid == null || uuid.isEmpty() || commands == null) {
                config = null;

                Log.e(TAG, "Could not load configuration");
            } else {
                config = new Configuration(uuid, name, commands); // Create Configuration from loaded data
            }
        } else { // Create a new configuration
            name = type.toString() + "-" + uuid.substring(0, 4); // TODO: prompt the user for a name
            config = new Configuration(uuid, name);
            getDatabase().insert(type, config);
        }

        label.setText(name); // Display the name of the configuration

        for (Map.Entry<Integer, View> entry : basicButtonMap.entrySet()) {
            // Add listeners to all buttons
            View button = entry.getValue();

            if (config != null) {
                button.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                        config.serializeCommand(entry.getKey(), true).getBytes()));
            }
        }

        RemoteRecyclerAdapter adapter;

        if (config != null) {
            recyclerView.setLayoutManager(new GridLayoutManager(this, 2,
                    LinearLayoutManager.HORIZONTAL, false));
            recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
            adapter = new RemoteRecyclerAdapter(this, type, config);
            recyclerView.setAdapter(adapter);

            WioMQTTClient.setCommandReceivedListener(payload -> {
                // When a command is received from MQTT, add it to the config
                Integer addedCommandKeyCode = config.addCommand(new String(payload.getPayloadAsBytes()));

                // If it is a command for a custom button, display a dialog requesting a name for the button
                if (addedCommandKeyCode != null && addedCommandKeyCode >= 0) {
                    int index = adapter.getItemCount();

                    Dialogs.requestInput(this, R.string.label, new Dialogs.OnConfirm() {
                        @Override
                        public void onConfirm(String text) {
                            Command command = config.getCommandForKeyCode(addedCommandKeyCode);
                            command.label = text;

                            getDatabase().update(type, config);

                            adapter.addCustomCommand(command);
                            adapter.notifyItemInserted(index);
                        }
                    }, () -> config.removeCommand(index));

                    adapter.hideDialog();
                } else {
                    getDatabase().update(type, config); // always update the DB if a basic command is recorded
                }
            });

            SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
            snapHelper.attachToRecyclerView(recyclerView);
        }


        // Set listener to update the DB every time the terminal exits cloning mode and the config changes
        WioMQTTClient.addTerminalModeListener(new WioMQTTClient.TerminalModeListener() {
            private Configuration prevConfig;

            @Override
            public void onEnteredCloningMode() {
                prevConfig = config;
            }

            @Override
            public void onExitedCloningMode() {
                if (config != null && !config.equals(prevConfig)) {
                    getDatabase().update(type, config);
                }
            }
        });
    }

    @Override
    protected void onResume() {
        if (update) {
            updateLayout();

            update = false;
        }

        super.onResume();
    }

    @Override
    protected void onStop() {
        preferences.edit()
                .putString(UUID, uuid)
                .putString(TYPE, type.toString())
                .apply(); // save current config to SharedPreferences to keep selection on next launch

        super.onStop();
    }

    @Override
    public int getStringResourceId() {
        return R.string.go_to_setup;
    }

    public static void updateConfiguration(ConfigurationType newType, String newUuid) {
        if (newType != null && newUuid != null &&
                (!newType.equals(type) || !newUuid.equals(uuid))) {
            type = newType;
            uuid = newUuid;

            update = true;
        }
    }

    public static void createConfiguration(Database database, String name) {
        Configuration configuration = new Configuration(java.util.UUID.randomUUID().toString(), name);

        database.insert(ConfigurationType.CUSTOM, configuration);

        type = ConfigurationType.CUSTOM;
        uuid = configuration.uuid;

        update = true;
    }

    public static boolean isLoaded(String uuidChecker) {
        return uuid != null && uuid.equals(uuidChecker);
    }
}
