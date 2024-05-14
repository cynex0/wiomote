package se.gu.wiomote.application.activities.remote;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.app.SearchManager;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import se.gu.wiomote.R;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.network.mqtt.WioMQTTClient;
import se.gu.wiomote.utils.Dialogs;
import se.gu.wiomote.utils.Utils;

public class RemoteRecyclerAdapter extends RecyclerView.Adapter<RemoteRecyclerAdapter.ViewHolder> {
    private static final String REQUEST_MODE_TOPIC = "wiomote/mode/request";
    private static final int ADD_BUTTON = 1;
    private static final int CUSTOM_BUTTON = 2;
    private final Configuration configuration;
    private final ConfigurationType type;
    private final List<Command> commands;
    private final Dialog waitingDialog;
    private final Remote activity;

    public RemoteRecyclerAdapter(Remote activity, ConfigurationType type, Configuration configuration) {
        this.activity = activity;
        this.configuration = configuration;
        this.type = type;
        this.commands = configuration.getCustomCommands();

        this.waitingDialog = new MaterialAlertDialogBuilder(activity)
                .setView(activity.getLayoutInflater().inflate(R.layout.waiting_dialog, null))
                .setCancelable(true)
                .setOnCancelListener(dialog ->
                        WioMQTTClient.publish(REQUEST_MODE_TOPIC, "EMIT".getBytes()))
                .setOnDismissListener(dialog ->
                        WioMQTTClient.publish(REQUEST_MODE_TOPIC, "EMIT".getBytes()))
                .create();

        WioMQTTClient.addTerminalModeListener(new WioMQTTClient.TerminalModeListener() {
            @Override
            public void onExitedCloningMode() {
                waitingDialog.dismiss();
            }
        });
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(activity);

        View itemView;

        if (viewType == CUSTOM_BUTTON) {
            itemView = inflater.inflate(R.layout.custom_button, parent, false);
        } else {
            itemView = inflater.inflate(R.layout.button_add, parent, false);

            itemView.setOnClickListener(v -> {
                WioMQTTClient.publish(REQUEST_MODE_TOPIC,
                        ("CLONE" + commands.size()).getBytes());

                waitingDialog.show();
            });
        }

        ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) itemView.getLayoutParams();
        layoutParams.width = (parent.getWidth() - parent.getPaddingLeft() -
                parent.getPaddingRight()) / 2 - layoutParams.leftMargin - layoutParams.rightMargin;
        itemView.setLayoutParams(layoutParams);

        return new ViewHolder(itemView, viewType);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, @SuppressLint("RecyclerView") int position) {
        if (holder.viewType == CUSTOM_BUTTON) {
            Command command = commands.get(position);

            if (command != null) {
                holder.label.setText(command.label);

                holder.itemView.setOnClickListener(view -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                        configuration.serializeCommand(position, true).getBytes()));

                holder.itemView.setOnLongClickListener(view -> {
                    Dialogs.displayDeleteConfirmation(activity, new Dialogs.OnConfirm() {
                        @Override
                        public void onConfirm() {
                            configuration.removeCommand(position);

                            activity.getDatabase()
                                    .update(type, configuration);

                            int index = commands.indexOf(command);

                            if (index >= 0) {
                                commands.remove(index);
                                notifyItemRemoved(index);
                            }
                        }
                    }, null);

                    return true;
                });
            }
        }
    }

    @Override
    public int getItemViewType(int position) {
        return getItemCount() - 1 == position ? ADD_BUTTON : CUSTOM_BUTTON;
    }

    @Override
    public int getItemCount() {
        return commands.size() + 1;
    }

    public void addCustomCommand(Command command) {
        commands.add(command);
    }

    public void hideDialog() {
        waitingDialog.dismiss();
    }

    protected static class ViewHolder extends RecyclerView.ViewHolder {
        private final int viewType;
        private TextView label;

        public ViewHolder(@NonNull View itemView, int viewType) {
            super(itemView);

            this.viewType = viewType;

            if (viewType == CUSTOM_BUTTON) {
                label = itemView.findViewById(R.id.label);
            }
        }
    }
}
