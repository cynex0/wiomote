package se.gu.wiomote.network;

import android.app.Application;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

import androidx.annotation.NonNull;

import java.util.ArrayList;

import se.gu.wiomote.utils.Utils;

public class WiFiHandler {
    private static final ArrayList<OnNetworkChanged> listeners = new ArrayList<>();
    private static WifiManager wifiManager;
    private static ConnectivityManager connectivityManager;

    public static void prepare(Application application) {
        wifiManager = (WifiManager) application.getSystemService(Context.WIFI_SERVICE);
        connectivityManager = (ConnectivityManager) application.getSystemService(Context.CONNECTIVITY_SERVICE);

        connectivityManager.registerDefaultNetworkCallback(new ConnectivityManager.NetworkCallback() {
            private static final int UNKNOWN = -1;
            private int currentCapability = UNKNOWN;

            @Override
            public void onCapabilitiesChanged(@NonNull Network network, @NonNull NetworkCapabilities networkCapabilities) {
                super.onCapabilitiesChanged(network, networkCapabilities);

                if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
                    if (currentCapability != NetworkCapabilities.TRANSPORT_WIFI) {
                        int oldCapability = currentCapability;
                        currentCapability = NetworkCapabilities.TRANSPORT_WIFI;

                        WifiInfo info = getWiFiInfo();

                        for (OnNetworkChanged listener : listeners) {
                            Utils.runOnUiThread(() -> {
                                if (oldCapability == UNKNOWN) {
                                    listener.onConnected();
                                }

                                if (info != null && info.getNetworkId() >= 0) {
                                    listener.onWiFiConnected(info);
                                } else {
                                    listener.onWiFiConnected(null);
                                }
                            });
                        }
                    }
                } else if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ||
                        networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
                    int oldCapability = currentCapability;
                    currentCapability = NetworkCapabilities.TRANSPORT_ETHERNET;

                    if (oldCapability == UNKNOWN) {
                        for (OnNetworkChanged listener : listeners) {
                            Utils.runOnUiThread(listener::onConnected);
                        }
                    }
                }
            }

            @Override
            public void onLost(@NonNull Network network) {
                super.onLost(network);

                currentCapability = UNKNOWN;

                for (OnNetworkChanged listener : listeners) {
                    Utils.runOnUiThread(() -> {
                        listener.onWiFiDisconnected();
                        listener.onDisconnected();
                    });
                }
            }
        });
    }

    private static WifiInfo getWiFiInfo() {
        if (wifiManager != null) {
            WifiInfo info = wifiManager.getConnectionInfo();

            if (info.getNetworkId() >= 0 &&
                    info.getSupplicantState() == SupplicantState.COMPLETED) {
                return info;
            }
        }

        return null;
    }

    public static void addOnNetworkChangedListener(OnNetworkChanged listener) {
        if (listener != null) {
            listeners.add(listener);

            Network network = connectivityManager.getActiveNetwork();

            if (network != null) {
                NetworkCapabilities networkCapabilities = connectivityManager.getNetworkCapabilities(network);

                if (networkCapabilities != null) {
                    if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
                        WifiInfo info = getWiFiInfo();

                        Utils.runOnUiThread(() -> {
                            listener.onConnected();

                            if (info != null && info.getNetworkId() >= 0) {
                                listener.onWiFiConnected(info);
                            } else {
                                listener.onWiFiConnected(null);
                            }
                        });

                        return;
                    } else if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ||
                            networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {

                        Utils.runOnUiThread(listener::onConnected);

                        return;
                    }
                }
            }

            Utils.runOnUiThread(() -> {
                listener.onWiFiDisconnected();
                listener.onDisconnected();
            });
        }
    }

    public static void removeOnNetworkChangedListener(OnNetworkChanged listener) {
        if (listener != null) {
            listeners.remove(listener);
        }
    }

    public interface OnNetworkChanged {
        default void onConnected() {
        }

        default void onDisconnected() {
        }

        default void onWiFiConnected(WifiInfo info) {
        }

        default void onWiFiDisconnected() {
        }
    }
}
