package se.gu.wiomote.activities;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;  // Handler for timer.

import androidx.appcompat.app.AppCompatActivity;

import se.gu.wiomote.R;
import se.gu.wiomote.activities.remote.Remote;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public class Main extends AppCompatActivity {
    private final Handler handler = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        Runnable runnable = () -> {
            startActivity(new Intent(Main.this, Setup.class));

            finish();
        };

        handler.postDelayed(runnable, WioMQTTClient.CONNECTION_TIMEOUT); // Delay while checking for connection.

        WioMQTTClient.setOnConnectionStatusChangedListener(new WioMQTTClient.OnConnectionStatusChanged() {
            @Override
            public void onConnected() {

                startActivity(new Intent(Main.this, Remote.class)); // Move to Remote activity if connection is established.

                finish();

                handler.removeCallbacks(runnable);
            }

            @Override
            public void onDisconnected() {
            }
        });
    }
}