package se.gu.wiomote.utils;

import se.gu.wiomote.configurations.Command;

public class CustomCommandJson {
    public String label;
    public String commandJson;

    public CustomCommandJson(String label, String command) {
        this.label = label;
        this.commandJson = command;
    }
}
