package se.gu.wiomote.application.activities.list;

import android.app.Activity;
import android.database.Cursor;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.shuhart.stickyheader.StickyAdapter;

import java.util.ArrayList;
import java.util.List;

import se.gu.wiomote.R;
import se.gu.wiomote.application.activities.remote.Remote;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;

public class ConfigurationRecyclerAdapter extends
        StickyAdapter<ConfigurationRecyclerAdapter.ViewHolder, ConfigurationRecyclerAdapter.ViewHolder> {
    private static final int LIST_ITEM = 0;
    private static final int HEADER_ITEM = 1;
    private final Activity activity;
    private final List<String> uuids;
    private final List<String> names;

    public ConfigurationRecyclerAdapter(Activity activity, Database database) {
        this.activity = activity;
        this.uuids = new ArrayList<>();
        this.names = new ArrayList<>();

        for (ConfigurationType type : ConfigurationType.values()) {
            uuids.add(null);
            names.add(type.toString());

            Cursor cursor = database.getAll(type, Database.Columns.UUID, Database.Columns.NAME);

            if (cursor != null) {
                int uuidIndex = cursor.getColumnIndex(Database.Columns.UUID.toString());
                int nameIndex = cursor.getColumnIndex(Database.Columns.NAME.toString());

                if (uuidIndex >= 0 && nameIndex >= 0) {
                    while (!cursor.isAfterLast()) {
                        uuids.add(cursor.getString(uuidIndex));
                        names.add(cursor.getString(nameIndex));

                        cursor.moveToNext();
                    }
                }

                cursor.close();
            }
        }
    }

    @NonNull
    @Override
    public ConfigurationRecyclerAdapter.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(activity);

        if (viewType == LIST_ITEM) {
            return new ViewHolder(inflater.inflate(R.layout.list_item, parent, false));
        } else {
            return new ViewHolder(inflater.inflate(R.layout.header_item, parent, false));
        }
    }

    @Override
    public ConfigurationRecyclerAdapter.ViewHolder onCreateHeaderViewHolder(ViewGroup parent) {
        LayoutInflater inflater = LayoutInflater.from(activity);

        return new ViewHolder(inflater.inflate(R.layout.header_item, parent, false));
    }

    @Override
    public void onBindViewHolder(@NonNull ConfigurationRecyclerAdapter.ViewHolder holder, int position) {
        String uuid = uuids.get(position);
        String name = names.get(position);

        holder.label.setText(name);

        if (uuid != null) {
            holder.itemView.setOnClickListener(v -> {
                Remote.updateConfiguration(
                        ConfigurationType.valueOf(names.get(getHeaderPositionForItem(position))), uuid);

                activity.finish();
            });
        } else {
            holder.itemView.setOnLongClickListener(null);
        }
    }

    @Override
    public void onBindHeaderViewHolder(ConfigurationRecyclerAdapter.ViewHolder holder, int headerPosition) {
        holder.label.setText(names.get(headerPosition));
    }

    @Override
    public int getItemCount() {
        return uuids.size();
    }

    @Override
    public int getItemViewType(int position) {
        return uuids.get(position) == null ? HEADER_ITEM : LIST_ITEM;
    }

    @Override
    public int getHeaderPositionForItem(int itemPosition) {
        for (int index = itemPosition; index >= 0; index--) {
            if (uuids.get(index) == null) {
                return index;
            }
        }

        return 0;
    }

    protected static class ViewHolder extends RecyclerView.ViewHolder {
        protected final TextView label;

        public ViewHolder(@NonNull View itemView) {
            super(itemView);

            label = itemView.findViewById(R.id.label);
        }
    }
}
