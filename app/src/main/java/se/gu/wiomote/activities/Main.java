package se.gu.wiomote.activities;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import se.gu.wiomote.R;

public class Main extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);

        findViewById(R.id.open).setOnClickListener(v -> {
            Intent intent = new Intent(Main.this, Setup.class);

            startActivity(intent);
        });

        findViewById(R.id.mqtt).setOnClickListener(v -> {
            startActivity(new Intent(Main.this, Mqtt.class));
        });
    }
}