package se.gu.wiomote.application.activities.remote;

import android.annotation.SuppressLint;
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
import se.gu.wiomote.configurations.Configuration;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.network.mqtt.WioMQTTClient;
import se.gu.wiomote.utils.Dialogs;

public class RemoteRecyclerAdapter extends RecyclerView.Adapter<RemoteRecyclerAdapter.ViewHolder> {
    private static final String REQUEST_MODE_TOPIC = "wiomote/mode/request";
    private static final int ADD_BUTTON = 1;
    private static final int CUSTOM_BUTTON = 2;
    private static final String EMIT_MODE = "EMIT";
    private static final String CLONE_MODE = "CLONE";
    private final Configuration configuration;
    private final ConfigurationType type;
    private final List<Configuration.Entry> data;
    private final Dialog waitingDialog;
    private final Remote activity;

    public RemoteRecyclerAdapter(Remote activity, ConfigurationType type, Configuration configuration) {
        this.activity = activity;
        this.configuration = configuration;
        this.type = type;
        this.data = configuration.getCustomCommands();

        this.waitingDialog = new MaterialAlertDialogBuilder(activity)
                .setView(activity.getLayoutInflater().inflate(R.layout.waiting_dialog, null))
                .setCancelable(true)
                .setOnCancelListener(dialog ->
                        WioMQTTClient.publish(REQUEST_MODE_TOPIC, EMIT_MODE.getBytes()))
                .setOnDismissListener(dialog ->
                        WioMQTTClient.publish(REQUEST_MODE_TOPIC, EMIT_MODE.getBytes()))
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
                        (CLONE_MODE + (data.size() > 0 ? data.get(data.size() - 1).keyCode + 1 : 0)).getBytes());

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
            Configuration.Entry entry = data.get(position);

            if (entry != null) {
                holder.label.setText(entry.command.label);

                holder.itemView.setOnClickListener(view -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                        configuration.serializeCommand(entry.keyCode, true).getBytes()));

                View delete = holder.itemView.findViewById(R.id.delete);
                delete.setOnClickListener(view -> Dialogs.displayDeleteConfirmation(activity,
                        new Dialogs.OnConfirm() {
                            @Override
                            public void onConfirm() {
                                configuration.removeCommand(entry.keyCode);

                                activity.getDatabase()
                                        .update(type, configuration);

                                int index = data.indexOf(entry);

                                if (index >= 0) {
                                    data.remove(index);
                                    notifyItemRemoved(index);
                                }
                            }
                        }, null));
            }
        }
    }

    @Override
    public int getItemViewType(int position) {
        return getItemCount() - 1 == position ? ADD_BUTTON : CUSTOM_BUTTON;
    }

    @Override
    public int getItemCount() {
        return data.size() + 1;
    }

    public void addCustomCommand(Configuration.Entry command) {
        data.add(command);
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
