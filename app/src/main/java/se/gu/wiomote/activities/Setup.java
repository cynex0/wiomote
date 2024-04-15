package se.gu.wiomote.activities;

import android.Manifest;
import android.animation.Animator;
import android.animation.LayoutTransition;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.google.android.material.button.MaterialButton;

import se.gu.wiomote.R;

public class Setup extends AppCompatActivity {
    private MaterialButton locationPermissionButton;
    private MaterialButton networkButton;
    private View setupContainer;

    @SuppressLint("Recycle")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.setup);

        locationPermissionButton = findViewById(R.id.location);
        networkButton = findViewById(R.id.network);
        ViewGroup container = findViewById(R.id.container);
        setupContainer = findViewById(R.id.setup_container);

        Animator translateUp = ObjectAnimator.ofPropertyValuesHolder(
                container, PropertyValuesHolder.ofFloat("alpha", -4f, 1f));
        translateUp.setInterpolator(new DecelerateInterpolator());

        Animator translateDown = ObjectAnimator.ofPropertyValuesHolder(
                container, PropertyValuesHolder.ofFloat("alpha", 1f, -4f));
        translateDown.setInterpolator(new DecelerateInterpolator());

        LayoutTransition layoutTransition = new LayoutTransition();
        layoutTransition.setAnimator(LayoutTransition.APPEARING, translateUp);
        layoutTransition.setAnimator(LayoutTransition.DISAPPEARING, translateDown);

        container.setLayoutTransition(layoutTransition);

        ActivityResultLauncher<String[]> locationPermissionRequest =
                registerForActivityResult(new ActivityResultContracts
                                .RequestMultiplePermissions(), result -> {
                            Boolean fineLocationGranted = result.getOrDefault(
                                    Manifest.permission.ACCESS_FINE_LOCATION, false);

                            if (fineLocationGranted != null && fineLocationGranted) {
                                locationPermissionButton.setEnabled(false);
                                setupContainer.setVisibility(View.VISIBLE);
                            } else {
                                locationPermissionButton.setEnabled(true);
                                setupContainer.setVisibility(View.INVISIBLE);
                            }
                        }
                );

        locationPermissionButton.setOnClickListener(v -> {
            String[] permissions;

            boolean fineGranted = checkPermissionGranted(Manifest.permission.ACCESS_FINE_LOCATION);
            boolean coarseGranted = checkPermissionGranted(Manifest.permission.ACCESS_COARSE_LOCATION);

            if (!fineGranted && coarseGranted) {
                permissions = new String[]{
                        Manifest.permission.ACCESS_FINE_LOCATION
                };
            } else {
                permissions = new String[]{
                        Manifest.permission.ACCESS_FINE_LOCATION
                };
            }

            locationPermissionRequest.launch(permissions);
        });

        networkButton.setOnClickListener(v -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startActivity(new Intent(Settings.Panel.ACTION_INTERNET_CONNECTIVITY));
            }
        });
    }

    @Override
    protected void onResume() {
        boolean fineGranted = checkPermissionGranted(Manifest.permission.ACCESS_FINE_LOCATION);

        locationPermissionButton.setEnabled(!fineGranted);
        setupContainer.setVisibility(fineGranted ? View.VISIBLE : View.GONE);

        WifiInfo wifiInfo = getWiFiInfo();
        networkButton.setVisibility(wifiInfo == null ||
                wifiInfo.getBSSID() == null ? View.VISIBLE : View.GONE);

        super.onResume();
    }

    private WifiInfo getWiFiInfo() {
        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        if (wifiManager != null) {
            return wifiManager.getConnectionInfo();
        }

        return null;
    }

    private boolean checkPermissionGranted(String permission) {
        return ContextCompat.checkSelfPermission(Setup.this, permission) == PackageManager.PERMISSION_GRANTED;
    }
}
