package se.gu.wiomote.application.activities.list;

import android.os.Bundle;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.shuhart.stickyheader.StickyHeaderItemDecorator;

import se.gu.wiomote.R;
import se.gu.wiomote.application.DatabaseAccessActivity;

public class ConfigurationList extends DatabaseAccessActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.configurations);

        RecyclerView recycler = findViewById(R.id.recycler);

        ConfigurationRecyclerAdapter adapter = new ConfigurationRecyclerAdapter(this, getDatabase());

        recycler.setLayoutManager(new LinearLayoutManager(this));
        recycler.setAdapter(adapter);

        new StickyHeaderItemDecorator(adapter).attachToRecyclerView(recycler);
    }
}