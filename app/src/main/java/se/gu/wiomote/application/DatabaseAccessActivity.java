package se.gu.wiomote.application;

import se.gu.wiomote.application.activities.NotificationTrayActivity;
import se.gu.wiomote.configurations.Database;

public class DatabaseAccessActivity extends NotificationTrayActivity {
    public Database getDatabase() {
        return ((WIOmote) getApplicationContext()).database;
    }
}
