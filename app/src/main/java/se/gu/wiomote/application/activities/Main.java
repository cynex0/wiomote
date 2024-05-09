package se.gu.wiomote.application.activities;

import android.content.Intent;
import android.os.Bundle;

import se.gu.wiomote.R;
import se.gu.wiomote.application.activities.remote.Remote;

public class Main extends NotificationTrayActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);

        findViewById(R.id.open).setOnClickListener(v -> {
            Intent intent = new Intent(Main.this, Setup.class);

            startActivity(intent);
        });

        findViewById(R.id.remote).setOnClickListener(v -> {
            startActivity(new Intent(Main.this, Remote.class));
        });
    }
}