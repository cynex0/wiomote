package se.gu.wiomote.activities;

import android.content.Intent;
import android.os.Bundle;

import se.gu.wiomote.R;
import se.gu.wiomote.activities.remote.Remote;

private static final int CONNECTION_TIMEOUT = 5000;
private Handler handler = new Handler();
private TextView messageTextView;
private boolean connectionEstablished = false;

@Override
protected void onCreate(Bundle savedInstanceState) {

    super.onCreate(savedInstanceState);
    setContentView(R.layout.main);

    statusTextView = findViewById(R.id.status_text_view);
    statusTextView.setText("Looking for terminal connection...");

    handler.postDelayed(this::checkConnection, CONNECTION_TIMEOUT);

}

private void switchToActivity() {

    if (connectionEstablished) {

        startActivity(new Intent(Main.this, Remote.class));

    } else {

        startActivity(new Intent(Main.this, Setup.class));
    }
}

private void checkConnection() {

    WioMQTTClient.setOnConnectionStatusChangedListener(new WioMQTTClient.OnConnectionStatusChanged() {

        @Override
        public void onConnected() {

            notificationContainer.setVisibility(View.GONE);
            switchToActivity(Remote.class);
            connectionEstablished(true);
        }

        @Override
        public void onDisconnected() {

            notification.setText(R.string.connection_lost);
            notificationContainer.setVisibility(View.VISIBLE);
            connectionEstablished(false);
            switchToActivity(Setup.class);
        }
    }

    protected void onDestroy(){
        super.onDestroy();
        handler.removeCallbacks(null);
    }
}