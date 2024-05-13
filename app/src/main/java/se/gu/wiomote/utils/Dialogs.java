package se.gu.wiomote.utils;

import android.app.Activity;
import android.app.Dialog;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.concurrent.atomic.AtomicReference;

import se.gu.wiomote.R;

public class Dialogs {
    public static void displayDeleteConfirmation(Activity activity,
                                                 OnConfirm onConfirm, Runnable onCancel) {
        AtomicReference<Dialog> dialog = new AtomicReference<>(null);

        View root = activity.getLayoutInflater().inflate(R.layout.remove_dialog, null);

        Button cancel = root.findViewById(R.id.cancel);
        Button remove = root.findViewById(R.id.remove);

        cancel.setOnClickListener(v -> {
            if(onCancel != null) {
                onCancel.run();
            }

            if (dialog.get() != null) {
                dialog.get().dismiss();
            }
        });

        remove.setOnClickListener(v -> {
            if(onConfirm != null) {
                onConfirm.onConfirm();
            }

            if (dialog.get() != null) {
                dialog.get().dismiss();
            }
        });

        dialog.set(new MaterialAlertDialogBuilder(activity)
                .setView(root)
                .setOnCancelListener(d -> {
                    if(onCancel != null) {
                        onCancel.run();
                    }
                })
                .create());

        dialog.get().show();
    }

    public static void requestInput(Activity activity, int hintResId,
                                    OnConfirm onConfirm, Runnable onCancel) {
        AtomicReference<Dialog> dialog = new AtomicReference<>(null);

        View root = activity.getLayoutInflater().inflate(R.layout.label_dialog, null);

        EditText editText = root.findViewById(R.id.text);
        Button cancel = root.findViewById(R.id.cancel);
        Button save = root.findViewById(R.id.save);

        editText.setHint(hintResId);

        editText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                save.setEnabled(s.length() > 0);
            }
        });

        cancel.setOnClickListener(v -> {
            if(onCancel != null) {
                onCancel.run();
            }

            if (dialog.get() != null) {
                dialog.get().dismiss();
            }
        });

        save.setOnClickListener(v -> {
            if(onConfirm != null) {
                onConfirm.onConfirm(editText.getText().toString());
            }

            if (dialog.get() != null) {
                dialog.get().dismiss();
            }
        });

        dialog.set(new MaterialAlertDialogBuilder(activity)
                .setView(root)
                .setOnCancelListener(d -> {
                    if(onCancel != null) {
                        onCancel.run();
                    }
                })
                .create());

        dialog.get().show();
    }

    public interface OnConfirm {
        default void onConfirm(){}
        default void onConfirm(String text){}
    }
}
