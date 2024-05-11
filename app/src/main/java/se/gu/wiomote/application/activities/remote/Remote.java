package se.gu.wiomote.application.activities.remote;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.database.Cursor;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

import se.gu.wiomote.R;
import se.gu.wiomote.application.DatabaseAccessActivity;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public class Remote extends DatabaseAccessActivity {
    static final String IR_SEND_TOPIC = "wiomote/ir/app";
    private static final String TAG = "se.gu.wiomote.remote";
    private final Map<Integer, View> basicButtonMap = new HashMap<>();

    @SuppressLint("NotifyDataSetChanged")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.remote);

        RecyclerView recyclerView = findViewById(R.id.recycler);
        View power = findViewById(R.id.power);
        View ok = findViewById(R.id.ok);
        View up = findViewById(R.id.up);
        View left = findViewById(R.id.left);
        View down = findViewById(R.id.down);
        View right = findViewById(R.id.right);

        basicButtonMap.put(-1, power);
        basicButtonMap.put(-2, up);
        basicButtonMap.put(-3, right);
        basicButtonMap.put(-4, down);
        basicButtonMap.put(-5, left);
        basicButtonMap.put(-6, ok);

        Configuration config;
        Cursor cursor = getDatabase().get(ConfigurationType.TV, "6c78c5f1-432a-4642-8de1-db2dc332506e");

        if (cursor != null) {
            String uuid = Database.getColumn(cursor, Database.Columns.UUID);
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
            }
        } else {
            config = new Configuration(UUID.randomUUID().toString());
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
            adapter = new RemoteRecyclerAdapter(this, config);
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

                    //TODO generalize
                    getDatabase().update(ConfigurationType.TV, config);

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
                    getDatabase().update(ConfigurationType.TV, config);
                }
            }
        });
    }
}
