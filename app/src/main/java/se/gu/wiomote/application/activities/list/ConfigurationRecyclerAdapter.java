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

import se.gu.wiomote.R;
import se.gu.wiomote.application.activities.remote.Remote;
import se.gu.wiomote.configurations.ConfigurationType;
import se.gu.wiomote.configurations.Database;
import se.gu.wiomote.utils.Dialogs;

public class ConfigurationRecyclerAdapter extends
        StickyAdapter<ConfigurationRecyclerAdapter.HeaderViewHolder, ConfigurationRecyclerAdapter.ViewHolder> {
    private static final int LIST_ITEM = 0;
    private static final int HEADER_ITEM = 1;
    private final Activity activity;
    private final List<String> uuids;
    private final List<ConfigurationType> types;
    private final List<String> names;
    private final Database database;

    public ConfigurationRecyclerAdapter(Activity activity, Database database) {
        this.activity = activity;
        this.uuids = new ArrayList<>();
        this.types = new ArrayList<>();
        this.names = new ArrayList<>();
        this.database = database;

        for (ConfigurationType type : ConfigurationType.values()) {
            uuids.add(null);
            types.add(type);
            names.add(type.toString());

            Cursor cursor = database.getAll(type, Database.Columns.UUID, Database.Columns.NAME);

            if (cursor != null) {
                int uuidIndex = cursor.getColumnIndex(Database.Columns.UUID.toString());
                int nameIndex = cursor.getColumnIndex(Database.Columns.NAME.toString());

                if (uuidIndex >= 0 && nameIndex >= 0) {
                    while (!cursor.isAfterLast()) {
                        uuids.add(cursor.getString(uuidIndex));
                        types.add(type);
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
            return new ItemViewHolder(inflater.inflate(R.layout.list_item, parent, false));
        } else {
            return new HeaderViewHolder(inflater.inflate(R.layout.header_item, parent, false));
        }
    }

    @Override
    public HeaderViewHolder onCreateHeaderViewHolder(ViewGroup parent) {
        LayoutInflater inflater = LayoutInflater.from(activity);

        View view = inflater.inflate(R.layout.header_item, parent, false);

        return new HeaderViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ConfigurationRecyclerAdapter.ViewHolder holder, int position) {
        String uuid = uuids.get(position);
        String name = names.get(position);

        holder.label.setText(name);

        if (holder instanceof ItemViewHolder) {
            ItemViewHolder itemViewHolder = (ItemViewHolder) holder;

            holder.itemView.setOnClickListener(v -> {
                Remote.updateConfiguration(
                        ConfigurationType.valueOf(names.get(getHeaderPositionForItem(position))), uuid);

                activity.finish();
            });

            if (ConfigurationType.CUSTOM.equals(types.get(position))) {
                itemViewHolder.delete.setVisibility(View.VISIBLE);

                itemViewHolder.delete.setOnClickListener(v -> Dialogs.displayDeleteConfirmation(activity, new Dialogs.OnConfirm() {
                    @Override
                    public void onConfirm() {
                        database.remove(ConfigurationType.valueOf(names.get(getHeaderPositionForItem(position))), uuid);

                        uuids.remove(position);
                        types.remove(position);
                        names.remove(position);

                        notifyItemRemoved(position);
                        notifyItemRangeChanged(position, uuids.size() - position);

                        // Choose the next configuration automatically to avoid re-writing deleted config to the DB
                        if (Remote.isLoaded(uuid)) {
                            int nextConfig = getFirstValidConfigIndex();
                            if (nextConfig >= 0) {
                                Remote.updateConfiguration(
                                        ConfigurationType.valueOf(
                                                names.get(getHeaderPositionForItem(nextConfig))), uuids.get(nextConfig)
                                );
                            } else {
                                activity.finish();
                            }
                        }
                    }
                }, null));
            } else {
                itemViewHolder.delete.setVisibility(View.GONE);
            }
        } else if (holder instanceof HeaderViewHolder) {
            HeaderViewHolder headerViewHolder = (HeaderViewHolder) holder;

            holder.itemView.setOnLongClickListener(null);

            if (ConfigurationType.CUSTOM.equals(types.get(position))) {
                headerViewHolder.add.setVisibility(View.VISIBLE);

                headerViewHolder.add.setOnClickListener(v -> {
                    Dialogs.requestInput(activity, R.string.label, new Dialogs.OnConfirm() {
                        @Override
                        public void onConfirm(String text) {
                            Remote.createConfiguration(database, text);

                            Log.i("TAG", "Creating new config...");

                            activity.finish();
                        }
                    }, null);
                });
            } else {
                headerViewHolder.add.setVisibility(View.GONE);
            }
        }
    }

    @Override
    public void onBindHeaderViewHolder(HeaderViewHolder holder, int headerPosition) {
        String name = names.get(headerPosition);

        holder.label.setText(name);

        if (ConfigurationType.CUSTOM.equals(types.get(headerPosition))) {
            holder.add.setVisibility(View.VISIBLE);
        } else {
            holder.add.setVisibility(View.GONE);
        }
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

    private int getFirstValidConfigIndex() {
        if (uuids.isEmpty()) {
            return -1;
        }

        int index = 0;

        do {
            if (uuids.get(index) != null) {
                return index;
            }

            index = index + 1;
        } while (index < getItemCount());

        return -1;
    }

    protected static class ViewHolder extends RecyclerView.ViewHolder {
        protected final TextView label;

        public ViewHolder(@NonNull View itemView) {
            super(itemView);

            label = itemView.findViewById(R.id.label);
        }
    }

    protected static class ItemViewHolder extends ViewHolder {
        protected final ImageView delete;

        public ItemViewHolder(@NonNull View itemView) {
            super(itemView);

            delete = itemView.findViewById(R.id.delete);
        }
    }

    protected static class HeaderViewHolder extends ViewHolder {
        protected final ImageView add;

        public HeaderViewHolder(@NonNull View itemView) {
            super(itemView);

            add = itemView.findViewById(R.id.addConfig);
        }
    }
}
