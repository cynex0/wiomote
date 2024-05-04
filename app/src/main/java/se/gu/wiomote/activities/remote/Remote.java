package se.gu.wiomote.activities.remote;

import android.os.Bundle;
import android.view.Gravity;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;

import se.gu.wiomote.R;
import se.gu.wiomote.network.WioMQTTClient;

public class Remote extends AppCompatActivity {
    static final String IR_SEND_TOPIC = "wiomote/ir/app";

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

        power.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Power button pressed".getBytes()));
        ok.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Ok button pressed".getBytes()));
        up.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Up button pressed".getBytes()));
        left.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Left button pressed".getBytes()));
        down.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Down button pressed".getBytes()));
        right.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Right button pressed".getBytes()));

        recyclerView.setLayoutManager(new GridLayoutManager(this, 2,
                LinearLayoutManager.HORIZONTAL, false));
        recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        recyclerView.setAdapter(new RemoteRecyclerAdapter(this));

        SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
        snapHelper.attachToRecyclerView(recyclerView);
    }
}
