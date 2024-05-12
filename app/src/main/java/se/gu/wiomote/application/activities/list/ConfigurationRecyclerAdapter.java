package se.gu.wiomote.application.activities.list;

import android.app.Activity;
import android.database.Cursor;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.shuhart.stickyheader.StickyAdapter;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import se.gu.wiomote.R;
import se.gu.wiomote.application.activities.remote.Remote;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;

public class ConfigurationRecyclerAdapter extends
        StickyAdapter<ConfigurationRecyclerAdapter.HeaderViewHolder, ConfigurationRecyclerAdapter.ItemViewHolder> {
    private static final int LIST_ITEM = 0;
    private static final int HEADER_ITEM = 1;
    private final Activity activity;
    private final List<String> uuids;
    private final List<String> names;
    private final Database database;

    public ConfigurationRecyclerAdapter(Activity activity, Database database) {
        this.activity = activity;
        this.uuids = new ArrayList<>();
        this.names = new ArrayList<>();
        this.database = database;

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
    public ItemViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(activity);

        if (viewType == LIST_ITEM) {
            return new ItemViewHolder(inflater.inflate(R.layout.list_item, parent, false));
        } else {
            return new HeaderViewHolder(inflater.inflate(R.layout.header_item, parent, false));
        }
    }

    @Override
    public HeaderViewHolder onCreateHeaderViewHolder(ViewGroup parent) {
        LayoutInflater inflater = LayoutInflater.from(activity);

        return new HeaderViewHolder(inflater.inflate(R.layout.header_item, parent, false));
    }

    @Override
    public void onBindViewHolder(@NonNull ItemViewHolder holder, int position) {
        String uuid = uuids.get(position);
        String name = names.get(position);

        holder.label.setText(name);

        if (uuid != null) {
            holder.itemView.setOnClickListener(v -> {
                Remote.updateConfiguration(
                        ConfigurationType.valueOf(names.get(getHeaderPositionForItem(position))), uuid);

                activity.finish();
            });
            holder.delete.setOnClickListener(v -> {
                database.remove(ConfigurationType.valueOf(names.get(getHeaderPositionForItem(position))), uuid);
                uuids.remove(position);
                names.remove(position);

                notifyItemRemoved(position);
                notifyItemRangeChanged(position, uuids.size() - position);

                // Choose the next configuration automatically to avoid re-writing deleted config to the DB
                // TODO: Needs a better solution
                int nextConfig = getNextConfigPosition(position);
                if (nextConfig >= 0) {
                    Remote.updateConfiguration(ConfigurationType.valueOf(names.get(getHeaderPositionForItem(nextConfig))), uuids.get(nextConfig));
                } else {
                    activity.finish();
                }
            });
        } else {
            holder.itemView.setOnLongClickListener(null);
            HeaderViewHolder headerViewHolder = (HeaderViewHolder) holder;
            headerViewHolder.add.setOnClickListener(v -> {
                String newUuid = UUID.randomUUID().toString();
                Remote.updateConfiguration(ConfigurationType.valueOf(name), newUuid);
                Log.i("TAG", "Creating new config " + name + newUuid);
                activity.finish();
            });
        }
    }

    @Override
    public void onBindHeaderViewHolder(HeaderViewHolder holder, int headerPosition) {
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

    private int getNextConfigPosition(int position) {
        for (int i = position; i < uuids.size(); i++) {
            if (uuids.get(i) != null) return i;
        }
        return -1;
    }

    protected static class ItemViewHolder extends RecyclerView.ViewHolder {
        protected final TextView label;
        protected final ImageView delete;

        public ItemViewHolder(@NonNull View itemView) {
            super(itemView);

            label = itemView.findViewById(R.id.label);
            delete = itemView.findViewById(R.id.delete);
        }
    }

    protected static class HeaderViewHolder extends ItemViewHolder {
        protected final ImageView add;

        public HeaderViewHolder(@NonNull View itemView) {
            super(itemView);

            add = itemView.findViewById(R.id.addConfig);
        }
    }
}
