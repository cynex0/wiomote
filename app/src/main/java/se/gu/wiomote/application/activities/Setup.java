package se.gu.wiomote.application.activities;

import android.Manifest;
import android.animation.Animator;
import android.animation.LayoutTransition;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.PackageManager;
import android.net.wifi.WifiInfo;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;

import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.ResolvableApiException;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationSettingsRequest;
import com.google.android.gms.location.LocationSettingsResponse;
import com.google.android.gms.location.LocationSettingsStatusCodes;
import com.google.android.gms.tasks.Task;
import com.google.android.material.button.MaterialButton;

import org.json.JSONException;
import org.json.JSONObject;

import se.gu.wiomote.R;
import se.gu.wiomote.application.activities.remote.Remote;
import se.gu.wiomote.network.WiFiHandler;
import se.gu.wiomote.network.WioBluetoothGattCallback;
import se.gu.wiomote.network.mqtt.WioMQTTClient;
import se.gu.wiomote.views.ListeningButton;

public class Setup extends NotificationTrayActivity {
    private static final String TAG = "se.gu.wiomote.Settings";
    private static final String SSID = "ssid";
    private static final String BSSID = "bssid";
    private static final String PASSWORD = "password";
    private static final int REQUEST_CHECK_SETTINGS = 372654;
    private static final int LOCATION_REQUEST_INTERVAL = 5000;
    private ActivityResultLauncher<String[]> bluetoothPermissionRequest;
    private ActivityResultLauncher<Intent> bluetoothEnableRequest;
    private WioBluetoothGattCallback bluetoothCallback;
    private ListeningButton locationButton;
    private ListeningButton bluetoothButton;
    private View bluetoothContainer;
    private ViewGroup networkContainer;
    private View networkInfo;
    private TextView ssid;
    private String bssid;
    private EditText password;

    @SuppressLint({"Recycle"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.setup);

        ViewGroup container = findViewById(R.id.container);
        locationButton = findViewById(R.id.location);

        bluetoothContainer = findViewById(R.id.bluetooth_container);
        bluetoothButton = findViewById(R.id.bluetooth);

        networkContainer = findViewById(R.id.network_container);
        MaterialButton networkButton = findViewById(R.id.network);

        networkInfo = findViewById(R.id.network_info);
        ssid = findViewById(R.id.ssid);
        password = findViewById(R.id.password);
        MaterialButton send = findViewById(R.id.send);

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
        networkContainer.setLayoutTransition(layoutTransition);

        ActivityResultLauncher<String[]> locationPermissionRequest =
                registerForActivityResult(new ActivityResultContracts
                                .RequestMultiplePermissions(), result -> {
                            Boolean fineLocationGranted = result.getOrDefault(
                                    Manifest.permission.ACCESS_FINE_LOCATION, false);

                            if (Boolean.TRUE.equals(fineLocationGranted)) {
                                locationButton.performClick();
                            } else {
                                locationButton.setEnabled(true);
                            }
                        }
                );

        locationButton.setOnClickListener(v -> {
            if (ContextCompat.checkSelfPermission(this,
                    Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                String[] permissions = new String[]{
                        Manifest.permission.ACCESS_FINE_LOCATION,
                        Manifest.permission.ACCESS_COARSE_LOCATION
                };

                locationPermissionRequest.launch(permissions);
            } else {
                requestLocationService();
            }
        });

        locationButton.setOnEnableListener(enabled ->
                bluetoothContainer.setVisibility(enabled ? View.GONE : View.VISIBLE));

        bluetoothPermissionRequest = Build.VERSION.SDK_INT >= 31 ? registerForActivityResult(
                new ActivityResultContracts.RequestMultiplePermissions(), result -> {
                    Boolean bluetoothConnectGranted = result.getOrDefault(
                            Manifest.permission.BLUETOOTH_CONNECT, false);
                    Boolean bluetoothScanGranted = result.getOrDefault(
                            Manifest.permission.BLUETOOTH_SCAN, false);

                    if (Boolean.TRUE.equals(bluetoothConnectGranted) &&
                            Boolean.TRUE.equals(bluetoothScanGranted)) {
                        startActivity(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE));
                    }
                }) : null;

        bluetoothEnableRequest = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
                result -> bluetoothButton.setEnabled(result.getResultCode() != Activity.RESULT_OK));

        bluetoothButton.setOnClickListener(v -> requestLocationService());

        bluetoothCallback = new WioBluetoothGattCallback(this,
                new WioBluetoothGattCallback.OnConnectionChange() {
                    @Override
                    public void onConnect(WioBluetoothGattCallback.GattWrapper gattWrapper) {
                        send.setOnClickListener(v -> {
                            JSONObject json = new JSONObject();

                            try {
                                json.put(SSID, ssid.getText());
                                json.put(BSSID, bssid);
                                json.put(PASSWORD, password.getText());

                                gattWrapper.sendData(json.toString());

                                send.setEnabled(false);
                                send.setText(R.string.connecting);
                            } catch (JSONException exception) {
                                Log.e(TAG, "onConnect: ", exception);
                            }
                        });

                        send.setEnabled(true);
                        send.setText(R.string.connect);
                    }

                    @Override
                    public void onDisconnect() {
                        send.setEnabled(false);
                        send.setText(R.string.waiting);
                        send.setOnClickListener(null);
                    }
                }, () -> bluetoothButton.setEnabled(true));

        WiFiHandler.OnNetworkChanged listener = new WiFiHandler.OnNetworkChanged() {
            @Override
            public void onWiFiConnected(WifiInfo info) {
                if (info != null) {
                    ssid.setText(info.getSSID().replace("\"", ""));
                    bssid = info.getBSSID();

                    networkInfo.setVisibility(View.VISIBLE);
                } else {
                    networkInfo.setVisibility(View.GONE);
                }
            }

            @Override
            public void onWiFiDisconnected() {
                networkInfo.setVisibility(View.GONE);
            }
        };

        bluetoothButton.setOnEnableListener(enabled -> {
            if (enabled) {
                WiFiHandler.removeOnNetworkChangedListener(listener);
                networkInfo.setVisibility(View.GONE);

                networkContainer.setVisibility(View.GONE);

                bluetoothCallback.stopScanning();
            } else {
                if (Build.VERSION.SDK_INT < 31 || ContextCompat.checkSelfPermission(this,
                        Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                    bluetoothCallback.startScanning();

                    WiFiHandler.addOnNetworkChangedListener(listener);

                    networkContainer.setVisibility(View.VISIBLE);
                } else {
                    bluetoothButton.setEnabled(false);

                    bluetoothCallback.stopScanning();
                }
            }
        });

        networkButton.setOnClickListener(v -> {
            if (Build.VERSION.SDK_INT >= 29) {
                startActivity(new Intent(Settings.Panel.ACTION_INTERNET_CONNECTIVITY));
            } else {
                startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
            }
        });

        locationButton.performClick();
    }

    private void requestLocationService() {
        LocationSettingsRequest.Builder builder = new LocationSettingsRequest.Builder()
                .addLocationRequest(new LocationRequest.Builder(LOCATION_REQUEST_INTERVAL).build());
        builder.setAlwaysShow(true);

        Task<LocationSettingsResponse> result = LocationServices
                .getSettingsClient(getApplicationContext())
                .checkLocationSettings(builder.build());

        result.addOnCompleteListener(task -> {
            try {
                LocationSettingsResponse response = task.getResult(ApiException.class);

                if (response != null) {
                    locationButton.setEnabled(false);

                    requestBluetoothService();
                }
            } catch (ResolvableApiException resolvableApiException) {
                if (resolvableApiException.getStatusCode() ==
                        LocationSettingsStatusCodes.RESOLUTION_REQUIRED) {
                    try {
                        resolvableApiException.startResolutionForResult(this, REQUEST_CHECK_SETTINGS);
                    } catch (IntentSender.SendIntentException ex) {
                        ex.printStackTrace();
                    }

                    locationButton.setEnabled(true);
                    bluetoothButton.setEnabled(true);
                } else {
                    Toast.makeText(this,
                            "Unable to turn on location on this device", Toast.LENGTH_LONG).show();

                    finish();
                }
            } catch (ApiException apiException) {
                Log.e(TAG, "onCreate: ", apiException);
            }
        });
    }

    private void requestBluetoothService() {
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this,
                        Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            if (Build.VERSION.SDK_INT >= 31) {
                String[] permissions;

                permissions = new String[]{
                        Manifest.permission.BLUETOOTH_CONNECT,
                        Manifest.permission.BLUETOOTH_SCAN,
                };

                bluetoothPermissionRequest.launch(permissions);
            }
        } else {
            bluetoothEnableRequest.launch(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE));
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_CHECK_SETTINGS) {
            locationButton.setEnabled(resultCode != Activity.RESULT_OK);

            if (resultCode == Activity.RESULT_OK) {
                requestBluetoothService();
            } else {
                bluetoothButton.setEnabled(true);
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        setOnConnectionStatusChangeListener(new WioMQTTClient.OnConnectionStatusChanged() {
            @Override
            public void onConnected() {
                startActivity(new Intent(Setup.this, Remote.class));

                finish();
            }

            @Override
            public void onDisconnected() {
            }
        });
    }

    @Override
    public int getStringResourceId() {
        return R.string.connection_lost;
    }

    @Override
    protected void onDestroy() {
        bluetoothCallback.unregister();

        super.onDestroy();
    }
}
