package se.gu.wiomote.activities;

import android.animation.LayoutTransition;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import se.gu.wiomote.R;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public abstract class NotificationTrayActivity extends AppCompatActivity {
    private View notificationContainer;
    private TextView notification;

    @Override
    public void setContentView(int layoutResID) {
        super.setContentView(R.layout.notification_tray_activity);

        LinearLayout root = findViewById(R.id.notificationTrayActivityRoot);
        notificationContainer = root.findViewById(R.id.notificationTrayActivityNotificationContainer);
        notification = root.findViewById(R.id.notificationTrayActivityNotification);

        getLayoutInflater().inflate(layoutResID, root, true);

        root.setLayoutTransition(new LayoutTransition());
    }

    @Override
    protected void onStart() {
        WioMQTTClient.setOnConnectionStatusChangedListener(new WioMQTTClient.OnConnectionStatusChanged() {
            @Override
            public void onConnected() {
                notificationContainer.setVisibility(View.GONE);
            }

            @Override
            public void onDisconnected() {
                notification.setText(R.string.connection_lost);
                notificationContainer.setVisibility(View.VISIBLE);
            }
        });

        super.onStart();
    }
}
