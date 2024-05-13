package se.gu.wiomote.configurations;

import static se.gu.wiomote.configurations.Database.Columns.COMMANDS;
import static se.gu.wiomote.configurations.Database.Columns.NAME;
import static se.gu.wiomote.configurations.Database.Columns.UUID;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

import androidx.annotation.NonNull;

import com.readystatesoftware.sqliteasset.SQLiteAssetHelper;

public class Database {
    private final Helper helper;
    private final SQLiteDatabase database;

    public Database(Context context) {
        this.helper = new Helper(context);
        this.database = helper.getWritableDatabase();
    }

    public boolean isOpen() {
        return database.isOpen();
    }

    public void close() {
        if (isOpen()) {
            helper.close();
        }
    }

    public void insert(ConfigurationType type, Configuration configuration) {
        ContentValues contentValue = new ContentValues();
        contentValue.put(UUID.string, configuration.uuid);
        contentValue.put(Columns.NAME.string, configuration.name);
        contentValue.put(COMMANDS.string, configuration.serializeConfig());

        database.insert(type.getTableName(), null, contentValue);
    }

    public Cursor getAll(ConfigurationType type, Columns... databaseColumns) {
        String[] columns = null;

        if (databaseColumns.length > 0) {
            columns = new String[databaseColumns.length];

            for (int index = 0; index < columns.length; index++) {
                columns[index] = databaseColumns[index].string;
            }
        }

        Cursor cursor = database.query(type.getTableName(), columns,
                null, null, null, null, NAME + " ASC");

        if (cursor != null) {
            cursor.moveToFirst();
        }

        return cursor;
    }

    public Cursor get(ConfigurationType type, String uuid, Columns... databaseColumns) {
        String[] columns = new String[]{UUID.string, NAME.string, COMMANDS.string};

        if (databaseColumns.length > 0) {
            columns = new String[databaseColumns.length];

            for (int index = 0; index < columns.length; index++) {
                columns[index] = databaseColumns[index].string;
            }
        }

        Cursor cursor = database.query(type.getTableName(), columns, UUID + "=?",
                new String[]{uuid}, null, null, null);

        if (cursor != null) {
            cursor.moveToFirst();
        }

        return cursor;
    }

    public boolean update(ConfigurationType type, Configuration configuration) {
        ContentValues contentValues = new ContentValues();

        contentValues.put(NAME.string, configuration.name);
        contentValues.put(COMMANDS.string, configuration.serializeConfig());

        return database.update(type.getTableName(), contentValues,
                UUID + "=?", new String[]{configuration.uuid}) > 0;
    }

    public void remove(ConfigurationType type, String uuid) {
        database.delete(type.getTableName(), UUID + "=?",
                new String[]{uuid});
    }

    public static String getColumn(Cursor cursor, Columns column) {
        if (cursor != null && cursor.getCount() > 0) {
            int index = cursor.getColumnIndex(column.string);

            if (index >= 0) {
                return cursor.getString(index);
            }
        }

        return null;
    }

    private static class Helper extends SQLiteAssetHelper {
        private static final String DB_NAME = "WIOMOTE.db";
        private static final int DB_VERSION = 1;

        public Helper(Context context) {
            super(context, DB_NAME, null, DB_VERSION);
        }
    }

    public enum Columns {
        UUID("UUID"),
        NAME("NAME"),
        COMMANDS("COMMANDS");

        private final String string;

        Columns(String string) {
            this.string = string;
        }

        @NonNull
        @Override
        public String toString() {
            return string;
        }
    }
}
