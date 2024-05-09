package se.gu.wiomote.activities.remote;

import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;

import com.github.rubensousa.gravitysnaphelper.GravitySnapHelper;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import se.gu.wiomote.R;
import se.gu.wiomote.activities.NotificationTrayActivity;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public class Remote extends NotificationTrayActivity {
    static final String IR_SEND_TOPIC = "wiomote/ir/app";
    private static final String TAG = "se.gu.wiomote.remote";
    private final Map<Integer, View> basicButtonMap = new HashMap<>();

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

        //Configuration config = Configuration.deserializeJSON("{\"uuid\":\"testconfig\",\"name\":\"Milla TV\",\"commands\":[{\"command\":{\"label\":\"POWER\",\"dataLength\":68,\"rawData\":[8874,4707,516,580,525,571,525,1739,524,572,523,574,521,575,521,575,520,576,519,1746,518,1747,516,580,516,1749,525,1740,524,1742,522,1743,521,1744,520,576,518,578,518,578,517,1748,516,580,525,571,524,572,524,572,523,1742,521,1744,520,1745,519,577,518,1748,516,1749,526,1740,524,1741,523,1000]},\"keyCode\":-1},{\"command\":{\"label\":\"UP\",\"dataLength\":68,\"rawData\":[8872,4708,473,623,472,624,471,1794,522,605,469,627,468,628,467,629,466,630,465,1800,464,1801,473,623,472,1793,471,1795,470,1795,469,1796,468,1798,466,630,465,631,464,632,474,622,473,624,471,625,471,1794,470,626,469,1797,467,1798,466,1799,465,1800,474,1791,473,1792,471,625,470,1795,469,1000]},\"keyCode\":-2},{\"command\":{\"label\":\"RIGHT\",\"dataLength\":68,\"rawData\":[8871,4708,472,624,471,625,470,1795,469,627,468,628,467,629,518,609,466,630,465,1800,474,1792,472,624,471,1793,471,1794,522,1775,468,1797,467,1799,518,609,465,1800,516,1781,472,624,471,625,470,626,470,626,469,627,468,1797,467,629,466,630,465,1800,474,1791,473,1792,471,1794,470,1795,469,1000]},\"keyCode\":-3},{\"command\":{\"label\":\"DOWN\",\"dataLength\":68,\"rawData\":[8873,4727,463,632,474,622,473,1793,523,604,470,626,469,627,469,627,468,628,467,1798,518,1779,464,632,474,1791,472,1793,471,1794,523,1774,521,1776,468,1797,519,608,519,609,465,631,517,611,516,611,474,1792,525,602,525,603,471,1794,523,1774,522,1775,521,1775,520,1777,518,609,465,1800,517,1000]},\"keyCode\":-4},{\"command\":{\"label\":\"LEFT\",\"dataLength\":68,\"rawData\":[8865,4714,466,630,518,609,464,1801,516,611,473,623,473,623,472,624,523,604,523,1773,469,1795,468,628,467,1798,518,1777,518,1778,516,1780,473,1792,524,1772,523,1773,522,1774,521,606,521,606,467,628,520,607,519,608,519,608,465,631,464,632,473,1791,525,1771,471,1794,523,1773,521,1775,520,1000]},\"keyCode\":-5},{\"command\":{\"label\":\"PRESS\",\"dataLength\":68,\"rawData\":[8869,4712,520,575,521,575,520,1745,518,578,517,579,516,580,516,580,525,571,524,1741,522,1743,521,575,521,1745,518,1746,518,1747,516,1749,515,1751,523,573,523,573,522,1743,521,575,520,576,519,577,518,1747,517,579,517,1749,514,1751,524,572,523,1743,522,1743,521,1745,519,577,518,1748,517,1000]},\"keyCode\":-6},{\"command\":{\"label\":\"CHANNEL UP\",\"dataLength\":68,\"rawData\":[8896,4684,548,579,464,632,473,1792,472,624,471,625,470,626,522,574,468,627,552,1745,551,1746,549,578,465,1801,516,1749,547,1750,513,1752,544,1753,554,574,553,574,521,576,519,577,550,578,549,578,549,579,548,579,474,1792,514,1751,544,1752,554,1743,553,1744,552,1744,551,1746,550,1747,548,1000]},\"keyCode\":0},{\"command\":{\"label\":\"CHANNEL DOWN\",\"dataLength\":68,\"rawData\":[8872,4707,526,570,525,571,524,1741,470,625,470,627,468,627,468,628,467,629,466,1799,465,1800,474,622,474,1791,473,1793,471,1794,469,1796,468,1797,467,1798,466,630,465,631,464,632,474,623,472,624,471,625,470,625,471,625,470,1795,468,1797,467,1798,465,1800,464,1801,473,1792,472,1793,471,1000]},\"keyCode\":1},{\"command\":{\"label\":\"VOLUME UP\",\"dataLength\":68,\"rawData\":[8872,4710,524,572,523,573,522,1774,522,606,520,576,520,576,466,630,465,631,464,1802,473,1792,472,624,471,1795,470,1795,469,1797,467,1799,465,1800,464,632,474,1791,525,602,472,624,524,604,470,626,470,626,469,627,468,1798,519,609,465,1800,517,1779,474,1791,473,1792,471,1794,523,1773,522,1000]},\"keyCode\":2},{\"command\":{\"label\":\"VOLUME DOWN\",\"dataLength\":68,\"rawData\":[8874,4706,474,622,473,623,472,1793,471,625,470,626,469,626,469,627,468,628,467,1798,465,1800,464,632,473,1792,472,1793,470,1795,469,1795,468,1798,466,1798,465,1800,464,632,473,623,525,602,472,624,471,625,470,626,469,627,468,627,467,1798,519,1778,464,1801,473,1792,472,1793,471,1794,470,1000]},\"keyCode\":3},{\"command\":{\"label\":\"MUTE\",\"dataLength\":68,\"rawData\":[8865,4713,466,630,465,631,464,1800,474,622,473,623,472,624,471,624,470,626,469,1795,521,1775,520,607,519,1778,465,1799,517,1779,516,1781,472,1792,524,1772,470,626,522,605,521,1776,467,629,518,609,518,609,518,610,464,632,473,1792,525,1771,524,603,523,1773,522,1774,521,1775,520,1776,466,1000]},\"keyCode\":4}]}");
        Configuration config = new Configuration(UUID.randomUUID().toString(), "Test");
        if (config == null) Log.e(TAG, "Could not load configuration");

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

        WioMQTTClient.setCommandReceivedListener(new WioMQTTClient.CommandReceivedListener() {
            @Override
            public void onCommandReceived(Mqtt3Publish payload) {
                if (config != null) {
                    config.addCommandFromJSON(new String(payload.getPayloadAsBytes()));
                }
            }
        });

        for (Map.Entry<Integer,View> entry : basicButtonMap.entrySet()) {
            View button = entry.getValue();
            if (config != null) {
                button.setOnClickListener(v -> {
                    WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                            config.serializeCommand(entry.getKey()).getBytes());
                });
            }
        }

        recyclerView.setLayoutManager(new GridLayoutManager(this, 2,
                LinearLayoutManager.HORIZONTAL, false));
        recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        recyclerView.setAdapter(new RemoteRecyclerAdapter(this));

        SnapHelper snapHelper = new GravitySnapHelper(Gravity.START);
        snapHelper.attachToRecyclerView(recyclerView);
    }
}
