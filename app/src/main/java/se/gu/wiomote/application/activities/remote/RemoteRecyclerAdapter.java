package se.gu.wiomote.application.activities.remote;

import android.app.Activity;
import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.List;

import se.gu.wiomote.R;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.network.mqtt.WioMQTTClient;

public class RemoteRecyclerAdapter extends RecyclerView.Adapter<RemoteRecyclerAdapter.ViewHolder> {
    private static final String REQUEST_CLONING_TOPIC = "wiomote/request/clone";
    private static final int ADD_BUTTON = 1;
    private static final int CUSTOM_BUTTON = 2;
    private final Configuration configuration;
    private final List<Command> commands;
    private final Dialog dialog;
    private final Activity activity;

    public RemoteRecyclerAdapter(Activity activity, Configuration configuration) {
        this.activity = activity;
        this.configuration = configuration;
        this.commands = configuration.getCustomCommands();

        this.dialog = new MaterialAlertDialogBuilder(activity)
                .setView(activity.getLayoutInflater().inflate(R.layout.waiting_dialog, null))
                .setCancelable(false)
                .create();
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
                WioMQTTClient.publish(REQUEST_CLONING_TOPIC,
                        String.valueOf(commands.size()).getBytes());

                dialog.show();
            });
        }

        ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) itemView.getLayoutParams();
        layoutParams.width = (parent.getWidth() - parent.getPaddingLeft() -
                parent.getPaddingRight()) / 2 - layoutParams.leftMargin - layoutParams.rightMargin;
        itemView.setLayoutParams(layoutParams);

        return new ViewHolder(itemView, viewType);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        if (holder.viewType == CUSTOM_BUTTON) {
            Command command = commands.get(position);

            if (command != null) {
                holder.label.setText(command.label);

                holder.itemView.setOnClickListener(view -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                        configuration.serializeCommand(position, true).getBytes()));
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
        dialog.dismiss();
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
