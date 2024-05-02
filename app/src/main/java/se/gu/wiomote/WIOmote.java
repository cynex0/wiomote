package se.gu.wiomote;

import android.app.Application;

import se.gu.wiomote.network.WiFiHandler;

public class WIOmote extends Application {
    @Override
    public void onCreate() {
        super.onCreate();

        WiFiHandler.prepare(this);
    }
}
