package se.gu.wiomote.application;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Application;
import android.os.Build;

import java.util.Arrays;

import se.gu.wiomote.configurations.Database;
import se.gu.wiomote.network.WiFiHandler;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public class WIOmote extends Application {
    Database database;

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override
    public void onCreate() {
        super.onCreate();

        WiFiHandler.prepare(this);
        WioMQTTClient.prepare();

        database = new Database(this);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            revokeSelfPermissionsOnKill(
                    Arrays.asList(Manifest.permission.ACCESS_COARSE_LOCATION,
                            Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_FINE_LOCATION,
                            Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN)
            );
        }
    }
}
