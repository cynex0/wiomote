package se.gu.wiomote.activities.remote;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;

import se.gu.wiomote.R;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.network.WioMQTTClient;

public class Remote extends AppCompatActivity {
    static final String IR_SEND_TOPIC = "wiomote/ir/app";
    private static final String TAG = "se.gu.wiomote.remote";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.remote);

        /*
        SharedPreferences sharedPref = this.getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("testconfig", "{\"uuid\":\"testconfig\",\"name\":\"Milla TV\",\"commands\":[{\"keyCode\":-1,\"command\":{\"label\":\"Power\",\"dataLength\":68,\"rawData\":[8900,4677,467,628,466,630,463,1801,472,624,470,626,468,627,467,629,464,632,473,1791,470,1794,468,628,466,1798,474,1791,471,1793,468,1796,465,1801,471,623,471,625,469,626,468,1797,465,630,474,622,472,623,471,625,469,1796,466,1799,464,1801,471,625,470,1795,467,1798,464,801,1472,1793,469,1000]}},{\"keyCode\":-2,\"command\":{\"label\":\"UP\",\"dataLength\":68,\"rawData\":[8899,4677,467,628,466,630,464,1800,546,581,471,625,469,626,468,628,466,630,464,1800,472,1793,469,627,467,1797,465,1799,473,1792,470,1794,468,1797,465,631,473,623,472,624,470,626,468,628,466,630,465,1799,473,623,471,1794,468,1796,466,1799,473,1791,471,1794,468,1796,466,630,464,1801,471,1000]}},{\"keyCode\":-5,\"command\":{\"label\":\"LEFT\",\"dataLength\":68,\"rawData\":[8894,4682,473,623,545,582,470,1795,551,576,466,630,464,632,473,622,545,582,544,1752,468,1796,465,631,474,1791,544,1752,552,1743,550,1746,474,1791,544,1751,552,1744,550,1746,546,581,545,582,470,625,553,575,551,575,550,578,464,632,472,623,472,1793,553,1743,466,1799,547,1749,545,1752,552,1000]}},{\"keyCode\":-3,\"command\":{\"label\":\"RIGHT\",\"dataLength\":68,\"rawData\":[8900,4678,467,629,465,630,464,1802,471,624,470,626,469,627,551,576,465,631,474,1791,471,1794,468,628,466,1799,473,1791,545,1751,469,1796,466,1799,547,580,472,1793,555,1742,466,630,464,632,473,622,472,624,470,625,469,1796,466,630,464,631,473,1792,470,1795,467,1798,464,1801,471,1793,469,1000]}},{\"keyCode\":-4,\"command\":{\"label\":\"DOWN\",\"dataLength\":68,\"rawData\":[8864,4733,463,632,473,623,471,1793,553,574,468,628,466,630,464,631,473,623,471,1793,553,1743,466,630,464,1800,472,1793,469,1795,551,1745,548,1748,472,1792,543,584,552,575,467,629,549,578,548,579,473,1791,544,583,553,574,468,1797,548,1748,546,1750,544,1751,553,1743,550,577,464,1801,545,1000]}},{\"keyCode\":-6,\"command\":{\"label\":\"OK\",\"dataLength\":68,\"rawData\":[8892,4683,471,624,470,626,468,1796,465,631,473,622,472,624,470,626,469,627,467,1797,464,1801,472,624,470,1795,467,1798,464,1801,472,1793,469,1796,466,630,464,632,473,1792,469,627,468,627,467,629,465,1800,472,623,471,1794,468,1796,465,631,474,1791,470,1794,468,1796,465,630,464,1800,471,1000]}}]}");
        editor.apply();

        Configuration config = Configuration.deserializeJSON(sharedPref.getString("testconfig", "{}"));
        */

        Configuration config = Configuration.deserializeJSON("{\"uuid\":\"testconfig\",\"name\":\"Milla TV\",\"commands\":[{\"keyCode\":-1,\"command\":{\"label\":\"Power\",\"dataLength\":68,\"rawData\":[8900,4677,467,628,466,630,463,1801,472,624,470,626,468,627,467,629,464,632,473,1791,470,1794,468,628,466,1798,474,1791,471,1793,468,1796,465,1801,471,623,471,625,469,626,468,1797,465,630,474,622,472,623,471,625,469,1796,466,1799,464,1801,471,625,470,1795,467,1798,464,801,1472,1793,469,1000]}},{\"keyCode\":-2,\"command\":{\"label\":\"UP\",\"dataLength\":68,\"rawData\":[8899,4677,467,628,466,630,464,1800,546,581,471,625,469,626,468,628,466,630,464,1800,472,1793,469,627,467,1797,465,1799,473,1792,470,1794,468,1797,465,631,473,623,472,624,470,626,468,628,466,630,465,1799,473,623,471,1794,468,1796,466,1799,473,1791,471,1794,468,1796,466,630,464,1801,471,1000]}},{\"keyCode\":-5,\"command\":{\"label\":\"LEFT\",\"dataLength\":68,\"rawData\":[8894,4682,473,623,545,582,470,1795,551,576,466,630,464,632,473,622,545,582,544,1752,468,1796,465,631,474,1791,544,1752,552,1743,550,1746,474,1791,544,1751,552,1744,550,1746,546,581,545,582,470,625,553,575,551,575,550,578,464,632,472,623,472,1793,553,1743,466,1799,547,1749,545,1752,552,1000]}},{\"keyCode\":-3,\"command\":{\"label\":\"RIGHT\",\"dataLength\":68,\"rawData\":[8900,4678,467,629,465,630,464,1802,471,624,470,626,469,627,551,576,465,631,474,1791,471,1794,468,628,466,1799,473,1791,545,1751,469,1796,466,1799,547,580,472,1793,555,1742,466,630,464,632,473,622,472,624,470,625,469,1796,466,630,464,631,473,1792,470,1795,467,1798,464,1801,471,1793,469,1000]}},{\"keyCode\":-4,\"command\":{\"label\":\"DOWN\",\"dataLength\":68,\"rawData\":[8864,4733,463,632,473,623,471,1793,553,574,468,628,466,630,464,631,473,623,471,1793,553,1743,466,630,464,1800,472,1793,469,1795,551,1745,548,1748,472,1792,543,584,552,575,467,629,549,578,548,579,473,1791,544,583,553,574,468,1797,548,1748,546,1750,544,1751,553,1743,550,577,464,1801,545,1000]}},{\"keyCode\":-6,\"command\":{\"label\":\"OK\",\"dataLength\":68,\"rawData\":[8892,4683,471,624,470,626,468,1796,465,631,473,622,472,624,470,626,469,627,467,1797,464,1801,472,624,470,1795,467,1798,464,1801,472,1793,469,1796,466,630,464,632,473,1792,469,627,468,627,467,629,465,1800,472,623,471,1794,468,1796,465,631,474,1791,470,1794,468,1796,465,630,464,1800,471,1000]}}]}");
        if (config == null) Log.e(TAG, "Could not load configuration");

        RecyclerView recyclerView = findViewById(R.id.recycler);
        View power = findViewById(R.id.power);
        View ok = findViewById(R.id.ok);
        View up = findViewById(R.id.up);
        View left = findViewById(R.id.left);
        View down = findViewById(R.id.down);
        View right = findViewById(R.id.right);

        power.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                config.getCommandForKeyCode(-1).serializeJSON().getBytes())); // Power
        ok.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                config.getCommandForKeyCode(-6).serializeJSON().getBytes())); // OK
        up.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                config.getCommandForKeyCode(-2).serializeJSON().getBytes())); // Up
        left.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                config.getCommandForKeyCode(-5).serializeJSON().getBytes())); // Left
        down.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                config.getCommandForKeyCode(-4).serializeJSON().getBytes())); // Down
        right.setOnClickListener(v -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                config.getCommandForKeyCode(-3).serializeJSON().getBytes())); // Right

        recyclerView.setLayoutManager(new GridLayoutManager(this, 2,
                LinearLayoutManager.HORIZONTAL, false));
        recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        recyclerView.setAdapter(new RemoteRecyclerAdapter(this));

        SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
        snapHelper.attachToRecyclerView(recyclerView);
    }
}
