package se.gu.wiomote.application;

import se.gu.wiomote.application.activities.NotificationTrayActivity;
import se.gu.wiomote.configurations.Database;

public abstract class DatabaseAccessActivity extends NotificationTrayActivity {
    public Database getDatabase() {
        WIOmote app = ((WIOmote) getApplicationContext());

        if (app.database == null || !app.database.isOpen()) {
            app.database = new Database(app);
        }

        return app.database;
    }
}
