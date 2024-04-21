package se.gu.wiomote.network;

import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

import androidx.annotation.NonNull;

import se.gu.wiomote.Utils;

public class WiFiHandler {
    private final WifiManager wifiManager;
    private OnWiFiNetwork listener;

    public WiFiHandler(Activity activity) {
        this.wifiManager = (WifiManager) activity.getSystemService(Context.WIFI_SERVICE);

        ((ConnectivityManager) activity.getSystemService(Context.CONNECTIVITY_SERVICE))
                .registerDefaultNetworkCallback(new ConnectivityManager.NetworkCallback() {
                    @Override
                    public void onCapabilitiesChanged(@NonNull Network network, @NonNull NetworkCapabilities networkCapabilities) {
                        super.onCapabilitiesChanged(network, networkCapabilities);

                        Utils.runOnUiThread(() -> {
                            if (listener != null) {
                                WifiInfo info = getWiFiInfo();

                                if (info != null && info.getNetworkId() >= 0) {
                                    listener.onWiFiConnected(info);
                                } else {
                                    listener.onWiFiDisconnected();
                                }
                            }
                        });
                    }

                    @Override
                    public void onLost(@NonNull Network network) {
                        super.onLost(network);

                        Utils.runOnUiThread(() -> {
                            if (listener != null) {
                                listener.onWiFiDisconnected();
                            }
                        });
                    }
                });
    }

    private WifiInfo getWiFiInfo() {
        WifiInfo info = wifiManager.getConnectionInfo();

        if (info.getNetworkId() >= 0 &&
                info.getSupplicantState() == SupplicantState.COMPLETED) {
            return info;
        }

        return null;
    }

    public void setOnWiFiNetworkListener(OnWiFiNetwork listener) {
        this.listener = listener;

        Utils.runOnUiThread(() -> {
            if (listener != null) {
                WifiInfo info = getWiFiInfo();

                if (info != null && info.getNetworkId() >= 0) {
                    listener.onWiFiConnected(info);
                } else {
                    listener.onWiFiDisconnected();
                }
            }
        });
    }

    public interface OnWiFiNetwork {
        void onWiFiConnected(WifiInfo info);

        void onWiFiDisconnected();
    }
}
