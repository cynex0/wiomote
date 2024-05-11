package se.gu.wiomote.configurations;

import androidx.annotation.NonNull;

public enum ConfigurationType {
    TV("TV"),
    PROJECTOR("PROJECTOR"),
    CUSTOM("CUSTOM");

    private final String string;

    ConfigurationType(String string) {
        this.string = string;
    }

    @NonNull
    @Override
    public String toString() {
        return string;
    }

    String getTableName() {
        return toString();
    }
}
