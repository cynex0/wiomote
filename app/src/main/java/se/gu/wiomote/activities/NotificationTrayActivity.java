package se.gu.wiomote.activities;

import android.animation.LayoutTransition;
import android.content.Intent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import se.gu.wiomote.R;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public abstract class NotificationTrayActivity extends AppCompatActivity {
    private WioMQTTClient.OnConnectionStatusChanged listener;
    private View notificationContainer;
    private TextView notification;

    @Override
    public void setContentView(int layoutResID) {
        super.setContentView(R.layout.notification_tray_activity);

        LinearLayout root = findViewById(R.id.notificationTrayActivityRoot);
        notificationContainer = root.findViewById(R.id.notificationTrayActivityNotificationContainer);
        notification = root.findViewById(R.id.notificationTrayActivityNotification);

        notificationContainer.setOnClickListener(v -> {   // Starts setup activity, if parent activity is not already Setup.
            if(!(NotificationTrayActivity.this instanceof Setup)) {
                startActivity(new Intent(NotificationTrayActivity.this, Setup.class));

                finish();
            }
        });

        getLayoutInflater().inflate(layoutResID, root, true);

        root.setLayoutTransition(new LayoutTransition());
    }

    @Override
    protected void onStart() {
        WioMQTTClient.setOnConnectionStatusChangedListener(new WioMQTTClient.OnConnectionStatusChanged() {
            @Override
            public void onConnected() {
                notificationContainer.setVisibility(View.GONE);

                if(listener != null) {
                    listener.onConnected();
                }
            }

            @Override
            public void onDisconnected() {
                notification.setText(getStringResourceId());
                notificationContainer.setVisibility(View.VISIBLE);

                if(listener != null) {
                    listener.onDisconnected();
                }
            }
        });

        super.onStart();
    }

    public void setOnConnectionStatusChangeListener(WioMQTTClient.OnConnectionStatusChanged listener) {
        this.listener = listener; // Due to the need of mqtt connection status callback by child activities
        // We create a new listener that will be called on the respective methods, that will be implemented by the child.
    }

    public abstract int getStringResourceId(); // Used to select the text displayed in the notification
}
