package se.gu.wiomote.application.activities.remote;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.ArrayList;
import java.util.List;

import se.gu.wiomote.R;
import se.gu.wiomote.configurations.Command;
import se.gu.wiomote.network.mqtt.WioMQTTClient;
import se.gu.wiomote.utils.CustomCommandJson;

public class RemoteRecyclerAdapter extends RecyclerView.Adapter<RemoteRecyclerAdapter.ViewHolder> {
    private static final String CUSTOM_BUTTON_REQUEST_TOPIC = "wiomote/mode/requestClone";
    private static final int ADD_BUTTON = 1;
    private static final int CUSTOM_BUTTON = 2;
    private final Activity activity;
    private List<CustomCommandJson> customCommands;

    public RemoteRecyclerAdapter(Activity activity) {
        this.activity = activity;
        this.customCommands = new ArrayList<>();
    }

    public RemoteRecyclerAdapter(Activity activity, List<CustomCommandJson> customCommands) {
        this.activity = activity;
        this.customCommands = customCommands;
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

            //TODO create layout and logic to receive signals; use WioMQTTClient subscribe()
            itemView.setOnClickListener(v -> new MaterialAlertDialogBuilder(activity)
                    .setMessage("Dialog test")
                    .create().show());
        }

        ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) itemView.getLayoutParams();
        layoutParams.width = (parent.getWidth() - parent.getPaddingLeft() - parent.getPaddingRight()) / 2 - layoutParams.leftMargin - layoutParams.rightMargin;
        itemView.setLayoutParams(layoutParams);

        return new ViewHolder(itemView, viewType);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        if (holder.viewType == CUSTOM_BUTTON) {
            holder.label.setText(customCommands.get(position).label);

            holder.itemView.setOnClickListener(view -> WioMQTTClient.publish(Remote.IR_SEND_TOPIC,
                    customCommands.get(position).commandJson.getBytes()));
        }
        else if (holder.viewType == ADD_BUTTON) {
            holder.itemView.setOnClickListener(view -> WioMQTTClient.publish(CUSTOM_BUTTON_REQUEST_TOPIC,
                    String.valueOf(customCommands.size()).getBytes()));
        }
    }

    @Override
    public int getItemViewType(int position) {
        return getItemCount() - 1 == position ? ADD_BUTTON : CUSTOM_BUTTON;
    }

    @Override
    public int getItemCount() {
        return customCommands.size() + 1;
    }

    public void updateCustomCommands(List<CustomCommandJson> newCommands) {
        customCommands = newCommands;
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
