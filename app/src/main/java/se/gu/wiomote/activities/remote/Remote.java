package se.gu.wiomote.activities.remote;

import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;

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

        ViewGroup wioButtons = findViewById(R.id.wio_buttons);
        RecyclerView recyclerView = findViewById(R.id.recycler);

        wioButtons.setOnClickListener(view -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                "Test button pressed remote".getBytes()));

        wioButtons.setClipToOutline(true);

        recyclerView.setLayoutManager(new GridLayoutManager(this, 2,
                LinearLayoutManager.HORIZONTAL, false));
        recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        recyclerView.setAdapter(new RemoteRecyclerAdapter(this));

        SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
        snapHelper.attachToRecyclerView(recyclerView);
    }
}
