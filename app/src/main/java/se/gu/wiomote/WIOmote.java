package se.gu.wiomote;

import android.app.Application;

import se.gu.wiomote.network.WiFiHandler;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public class WIOmote extends Application {
    @Override
    public void onCreate() {
        super.onCreate();

        WiFiHandler.prepare(this);
        WioMQTTClient.prepare();
    }
}
