package se.gu.wiomote.configurations;

import androidx.annotation.NonNull;

public enum ConfigurationType {
    CUSTOM("CUSTOM"),
    TV("TV"),
    PROJECTOR("PROJECTOR");

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
