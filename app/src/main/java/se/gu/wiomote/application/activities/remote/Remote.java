package se.gu.wiomote.application.activities.remote;

import android.database.Cursor;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import se.gu.wiomote.R;
import se.gu.wiomote.application.DatabaseAccessActivity;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;
import se.gu.wiomote.network.mqtt.WioMQTTClient;
import se.gu.wiomote.utils.CustomCommandJson;

public class Remote extends DatabaseAccessActivity {
    static final String IR_SEND_TOPIC = "wiomote/ir/app";
    private static final String TAG = "se.gu.wiomote.remote";
    private final Map<Integer, View> basicButtonMap = new HashMap<>();

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

        Cursor cursor = getDatabase().get(ConfigurationType.TV, "6c78c5f1-432a-4642-8de1-db2dc332506e");

        if (cursor != null) {
            String uuid = Database.getColumn(cursor, Database.Columns.UUID);
            String name = Database.getColumn(cursor, Database.Columns.NAME);
            Map<Integer, Command> commands = Configuration.deserializeCommands(
                    Database.getColumn(cursor, Database.Columns.COMMANDS)
            );

            cursor.close();

            Configuration config;

            if (uuid == null || uuid.isEmpty() || commands == null) {
                config = null;

                Log.e(TAG, "Could not load configuration");
            } else {
                config = new Configuration(uuid, name, commands);
            }

            basicButtonMap.put(-1, power);
            basicButtonMap.put(-2, up);
            basicButtonMap.put(-3, right);
            basicButtonMap.put(-4, down);
            basicButtonMap.put(-5, left);
            basicButtonMap.put(-6, ok);

            WioMQTTClient.setCommandReceivedListener(payload -> {
                if (config != null) {
                    config.addCommand(new String(payload.getPayloadAsBytes()));
                }
            });

            for (Map.Entry<Integer, View> entry : basicButtonMap.entrySet()) {
                View button = entry.getValue();

                if (config != null) {
                    button.setOnClickListener(v -> {
                        WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                                config.serializeCommand(entry.getKey(), true).getBytes());
                    });
                }
            }

            if (config != null) {
                recyclerView.setLayoutManager(new GridLayoutManager(this, 2,
                        LinearLayoutManager.HORIZONTAL, false));
                recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
                recyclerView.setAdapter(new RemoteRecyclerAdapter(this, config.getCustomCommands()));

                SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
                snapHelper.attachToRecyclerView(recyclerView);
            }
        }
    }
}
