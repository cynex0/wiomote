package se.gu.wiomote.activities;

import android.os.Bundle;
import android.widget.EditText;

import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;

import se.gu.wiomote.R;
import se.gu.wiomote.network.WioMqttClient;

public class Mqtt extends AppCompatActivity {
    WioMqttClient mqttClient;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mqtt);

        mqttClient = new WioMqttClient();
        EditText mqttMessage = findViewById(R.id.mqttMessage);
        MaterialButton sendMqtt = findViewById(R.id.mqttSend);

        sendMqtt.setOnClickListener(v -> mqttClient.publish(String.valueOf(mqttMessage.getText())));
    }
}
