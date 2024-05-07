package se.gu.wiomote.network;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.util.Log;

import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.UUID;

import se.gu.wiomote.utils.Utils;

public class WioBluetoothGattCallback extends BluetoothGattCallback {
    private static final String TAG = "se.gu.wiomote.WioBluetoothGattCallback";
    private static final String WIO_ID_SUFFIX = "WIOmote";
    private static final UUID SERVICE_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    private static final UUID CHARACTERISTIC_UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    private static final ArrayList<BluetoothDevice> devices = new ArrayList<>();
    private final Activity activity;
    private final BroadcastReceiver bluetoothDeviceFoundReceiver;
    private final BroadcastReceiver bluetoothConnectionStateReceiver;
    private final BroadcastReceiver bluetoothDiscoveryReceiver;
    private final OnConnectionChange listener;
    private boolean scan;
    private boolean connected;

    public WioBluetoothGattCallback(Activity activity, OnConnectionChange listener,
                                    OnAdapterDisconnect stateChange) {
        this.activity = activity;
        this.listener = listener;
        this.scan = false;
        this.connected = false;

        bluetoothDeviceFoundReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                if (ContextCompat.checkSelfPermission(context,
                        Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED &&
                        BluetoothDevice.ACTION_FOUND.equals(intent.getAction())) {

                    BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

                    if (device != null) {
                        String deviceName = device.getName();

                        if (deviceName != null && deviceName.startsWith(WIO_ID_SUFFIX)) {
                            devices.add(device);

                            device.connectGatt(context, false, WioBluetoothGattCallback.this);
                        }
                    }
                }
            }
        };

        activity.registerReceiver(bluetoothDeviceFoundReceiver,
                new IntentFilter(BluetoothDevice.ACTION_FOUND));

        bluetoothConnectionStateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1) ==
                        BluetoothAdapter.STATE_OFF) {
                    Utils.runOnUiThread(() -> {
                        if (stateChange != null) {
                            stateChange.onDisconnect();
                        }

                        connected = false;

                        if (listener != null) {
                            Utils.runOnUiThread(listener::onDisconnect);
                        }
                    });
                }
            }
        };

        activity.registerReceiver(bluetoothConnectionStateReceiver,
                new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));

        bluetoothDiscoveryReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (scan && !connected) {
                    startScanning();
                }
            }
        };

        activity.registerReceiver(bluetoothDiscoveryReceiver,
                new IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_FINISHED));
    }

    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
        if (newState == BluetoothProfile.STATE_CONNECTED) {
            if (ContextCompat.checkSelfPermission(activity,
                    Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {

                gatt.discoverServices();
            }
        } else {
            if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                BluetoothDevice device = gatt.getDevice();

                devices.remove(gatt.getDevice());
                devices.add(device);
            }

            if (listener != null && connected) {
                connected = false;

                if (scan) {
                    startScanning();
                }

                Utils.runOnUiThread(listener::onDisconnect);
            }
        }
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        BluetoothGattService service = gatt.getService(SERVICE_UUID);

        if (service != null) {
            BluetoothGattCharacteristic characteristic =
                    service.getCharacteristic(CHARACTERISTIC_UUID);

            if (characteristic != null) {
                if (listener != null && !connected) {
                    connected = true;

                    Utils.runOnUiThread(() -> listener.onConnect(new GattWrapper(gatt, characteristic)));
                }
            }
        }
    }

    public void unregister() {
        try {
            activity.unregisterReceiver(bluetoothDeviceFoundReceiver);
        } catch (RuntimeException exception) {
            Log.e(TAG, "onDestroy: Receiver not registered.");
        }

        try {
            activity.unregisterReceiver(bluetoothConnectionStateReceiver);
        } catch (RuntimeException exception) {
            Log.e(TAG, "onDestroy: Receiver not registered.");
        }

        try {
            activity.unregisterReceiver(bluetoothDiscoveryReceiver);
        } catch (RuntimeException exception) {
            Log.e(TAG, "onDestroy: Receiver not registered.");
        }
    }

    public void startScanning() {
        scan = true;

        if (ContextCompat.checkSelfPermission(activity,
                Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
            BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();

            if (devices.size() > 0) {
                devices.get(0).connectGatt(activity, false, this);
            }

            if (!adapter.isDiscovering()) {
                Log.d(TAG, "Starting discovery...");

                adapter.startDiscovery();
            }
        }
    }

    public void stopScanning() {
        scan = false;

        if (ContextCompat.checkSelfPermission(activity,
                Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
            BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
        }
    }

    public interface OnConnectionChange {
        void onConnect(GattWrapper gattWrapper);

        void onDisconnect();
    }

    public interface OnAdapterDisconnect {
        void onDisconnect();
    }

    public class GattWrapper {
        private final BluetoothGatt gatt;
        private final BluetoothGattCharacteristic characteristic;

        public GattWrapper(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            this.gatt = gatt;
            this.characteristic = characteristic;
        }

        public void sendData(String data) {
            if (ContextCompat.checkSelfPermission(activity,
                    Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
                characteristic.setValue(data);

                gatt.writeCharacteristic(characteristic);
            }
        }
    }
}
