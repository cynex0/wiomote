package se.gu.wiomote.configurations;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Database {
    private final Helper helper;
    private final SQLiteDatabase database;

    public Database(Context context) {
        this.helper = new Helper(context);
        this.database = helper.getWritableDatabase();
    }

    public void close() {
        helper.close();
    }

    public void insert(ConfigurationType type, String uuid, String json) {
        ContentValues contentValue = new ContentValues();
        contentValue.put(Helper.UUID, uuid);
        contentValue.put(Helper.JSON, json);

        database.insert(type.getTableName(), null, contentValue);
    }

    public Cursor get(ConfigurationType type) {
        String[] columns = new String[]{Helper.UUID, Helper.JSON};

        Cursor cursor = database.query(type.getTableName(), columns,
                null, null, null, null, null);

        if (cursor != null) {
            cursor.moveToFirst();
        }

        return cursor;
    }

    public String get(ConfigurationType type, String uuid) {
        String[] columns = new String[]{Helper.UUID, Helper.JSON};

        Cursor cursor = database.query(type.getTableName(), columns, Helper.UUID + "=?",
                new String[]{uuid}, null, null, null);

        if (cursor != null) {
            cursor.moveToFirst();

            int index = cursor.getColumnIndex(Helper.JSON);

            if (index >= 0 && cursor.getCount() > 0) {
                String json = cursor.getString(index);

                cursor.close();

                return json;
            } else {
                cursor.close();
            }
        }

        return null;
    }

    public boolean update(ConfigurationType type, String uuid, String json) {
        ContentValues contentValues = new ContentValues();
        contentValues.put(Helper.UUID, uuid);
        contentValues.put(Helper.JSON, json);

        return database.update(type.getTableName(), contentValues,
                Helper.UUID + "=?", new String[]{uuid}) > 0;
    }

    public void remove(ConfigurationType type, String uuid) {
        database.delete(type.getTableName(), Helper.UUID + "=?",
                new String[]{uuid});
    }

    private static class Helper extends SQLiteOpenHelper {
        private static final String DB_NAME = "WIOMOTE";
        private static final int DB_VERSION = 1;
        public static final String UUID = "UUID";
        public static final String JSON = "JSON";
        private static final String TAG = "DatabaseHelper";
        private final Context context;
        private boolean createDatabase;

        public Helper(Context context) {
            super(context, DB_NAME, null, DB_VERSION);

            this.context = context;
            this.createDatabase = false;
        }

        @Override
        public void onCreate(SQLiteDatabase database) {
            createDatabase = true;
        }

        @Override
        public void onUpgrade(SQLiteDatabase database, int oldVersion, int newVersion) {
            for (ConfigurationType type : ConfigurationType.values()) {
                database.execSQL("DROP TABLE IF EXISTS " + type.getTableName());
            }

            onCreate(database);
        }

        // modification of https://stackoverflow.com/a/29281714
        @Override
        public void onOpen(SQLiteDatabase database) {
            if(createDatabase) {
                createDatabase = false;

                InputStream inputStream = null;
                OutputStream outputStream = null;

                try {
                    inputStream = context.getAssets().open(DB_NAME + ".db");
                    outputStream = Files.newOutputStream(Paths.get(database.getPath()));

                    byte[] buffer = new byte[1024];
                    int length;

                    while ((length = inputStream.read(buffer)) > 0) {
                        Log.i(TAG, "onCreate: " + new String(buffer));
                        outputStream.write(buffer, 0, length);
                    }

                    outputStream.flush();

                    SQLiteDatabase copiedDatabase = context.openOrCreateDatabase(DB_NAME, Context.MODE_PRIVATE, null);

                    copiedDatabase.execSQL("PRAGMA user_version = " + DB_VERSION);
                    copiedDatabase.close();
                } catch (IOException exception) {
                    for (ConfigurationType type : ConfigurationType.values()) {
                        database.execSQL("CREATE TABLE " + type.getTableName() + "(" +
                                UUID + " VARCHAR(36)," +
                                JSON + " MEDIUMTEXT" +
                                ")"
                        );
                    }
                } finally {
                    try {
                        if (outputStream != null) {
                            outputStream.flush();
                            outputStream.close();
                        }

                        if (inputStream != null) {
                            inputStream.close();
                        }
                    } catch (IOException exception) {
                        Log.e(TAG, "onCreate: Exception closing streams.");
                    }
                }
            }
        }
    }
}
