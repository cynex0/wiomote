package se.gu.wiomote.application.activities.remote;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

import se.gu.wiomote.R;
import se.gu.wiomote.application.DatabaseAccessActivity;
import se.gu.wiomote.application.activities.list.ConfigurationList;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

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
        Configuration config;
        Cursor cursor = getDatabase().get(type, uuid);

        if (cursor != null) {
            String name = Database.getColumn(cursor, Database.Columns.NAME);
            Map<Integer, Command> commands = Configuration.deserializeCommands(
                    Database.getColumn(cursor, Database.Columns.COMMANDS)
            );

            cursor.close();

            if (uuid == null || uuid.isEmpty() || commands == null) {
                config = null;

                Log.e(TAG, "Could not load configuration");
            } else {
                config = new Configuration(uuid, name, commands);

                label.setText(name);
            }
        } else {
            config = new Configuration(java.util.UUID.randomUUID().toString());
        }

        for (Map.Entry<Integer, View> entry : basicButtonMap.entrySet()) {
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
                Command command = config.addCommand(new String(payload.getPayloadAsBytes()));
                int index = adapter.getItemCount();

                AtomicReference<Dialog> dialog = new AtomicReference<>(null);

                View root = getLayoutInflater().inflate(R.layout.label_dialog, null);

                EditText editText = root.findViewById(R.id.text);
                Button cancel = root.findViewById(R.id.cancel);
                Button save = root.findViewById(R.id.save);

                editText.addTextChangedListener(new TextWatcher() {
                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                        save.setEnabled(s.length() > 0);
                    }
                });

                cancel.setOnClickListener(v -> {
                    config.removeCommand(index);

                    if (dialog.get() != null) {
                        dialog.get().dismiss();
                    }
                });

                save.setOnClickListener(v -> {
                    command.label = editText.getText().toString();

                    getDatabase().update(type, config);

                    adapter.addCustomCommand(command);
                    adapter.notifyItemInserted(index);

                    if (dialog.get() != null) {
                        dialog.get().dismiss();
                    }
                });


                dialog.set(new MaterialAlertDialogBuilder(this).setView(root)
                        .setOnCancelListener(d -> config.removeCommand(index))
                        .create());

                dialog.get().show();

                adapter.hideDialog();
            });

            SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
            snapHelper.attachToRecyclerView(recyclerView);
        }

        WioMQTTClient.setTerminalModeListener(new WioMQTTClient.TerminalModeListener() {
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
                .apply();

        super.onStop();
    }

    public static void updateConfiguration(ConfigurationType newType, String newUuid) {
        if (newType != null && newUuid != null &&
                (!newType.equals(type) || !newUuid.equals(uuid))) {
            type = newType;
            uuid = newUuid;

            update = true;
        }
    }
}
