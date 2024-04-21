package se.gu.wiomote.activities;

import android.os.Bundle;
import android.os.PersistableBundle;
import android.widget.EditText;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;

import se.gu.wiomote.R;
import se.gu.wiomote.WioMqttClient;

public class Mqtt extends AppCompatActivity {
    private EditText mqttMessage;
    private MaterialButton sendMqtt;
    WioMqttClient mqttClient;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mqtt);

        mqttClient = new WioMqttClient();
        mqttMessage = findViewById(R.id.mqttMessage);
        sendMqtt = findViewById(R.id.mqttSend);

        sendMqtt.setOnClickListener(v -> {
            mqttClient.publish(String.valueOf(mqttMessage.getText()));
        });
        sendMqtt.setEnabled(true);
    }
}
