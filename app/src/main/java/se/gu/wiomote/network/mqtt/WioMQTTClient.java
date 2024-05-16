package se.gu.wiomote.network.mqtt;

import android.util.Log;

import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedContext;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedListener;
import com.hivemq.client.mqtt.mqtt3.Mqtt3AsyncClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3Client;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;
import com.hivemq.client.mqtt.mqtt3.message.subscribe.suback.Mqtt3SubAck;

import org.jetbrains.annotations.NotNull;

import java.util.ArrayList;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;

import se.gu.wiomote.network.WiFiHandler;
import se.gu.wiomote.utils.TimeoutBoolean;
import se.gu.wiomote.utils.Utils;

public class WioMQTTClient {
    public static final int CONNECTION_TIMEOUT = 2469;
    private static final String TAG = "se.gu.wiomote.MQTT";
    private static final String ID_SUFFIX = "WIOmote_app-";
    private static final String HOST = "broker.hivemq.com";
    private static final int PORT = 1883;
    private static final String CONN_IN_TOPIC = "wiomote/connection/terminal";
    private static final String IR_IN_TOPIC = "wiomote/ir/terminal";
    private static final String TERMINAL_MODE_TOPIC = "wiomote/mode";
    private static final TimeoutBoolean connected = new TimeoutBoolean(CONNECTION_TIMEOUT);
    private static final Mqtt3AsyncClient client = Mqtt3Client.builder()
            .identifier(ID_SUFFIX + UUID.randomUUID())
            .serverHost(HOST)
            .serverPort(PORT)
            .automaticReconnectWithDefaultConfig()
            .addConnectedListener(new MqttClientConnectedListener() {
                @Override
                public void onConnected(@NotNull MqttClientConnectedContext context) {
                    state = ConnectionStatus.CONNECTED;
                }
            })
            .build()
            .toAsync();

    private static WioMQTTClient.OnConnectionStatusChanged connectionListener = null;
    private static CommandReceivedListener commandListener = null;
    private static ArrayList<TerminalModeListener> terminalModeListeners = new ArrayList<>();
    private static ConnectionStatus state = ConnectionStatus.UNKNOWN;
    private static boolean ready = false;

    public static void prepare() {
        if (!ready) {
            connected.setOnUpdateListener(value ->
                    Utils.runOnUiThread(() -> {
                        if (connectionListener != null) {
                            if (value) {
                                connectionListener.onConnected();
                            } else {
                                connectionListener.onDisconnected();
                            }
                        }
                    }));

            WiFiHandler.addOnNetworkChangedListener(new WiFiHandler.OnNetworkChanged() {

                @Override
                public void onConnected() {
                    if (state == ConnectionStatus.UNKNOWN) {
                        client.connect()
                                .thenCompose(connAck -> subscribe(CONN_IN_TOPIC, payload -> connected.setTrue()))
                                .thenCompose(mqtt3SubAck -> subscribe(IR_IN_TOPIC, payload ->
                                        Utils.runOnUiThread(() ->
                                                commandListener.onCommandReceived(payload))))
                                .thenCompose(mqtt3SubAck -> subscribe(TERMINAL_MODE_TOPIC, payload -> {
                                    String mode = new String(payload.getPayloadAsBytes());
                                    for (TerminalModeListener listener : terminalModeListeners) {
                                        if ("EMIT".equals(mode))
                                            listener.onExitedCloningMode();
                                        else if ("CLONE".equals(mode))
                                            listener.onEnteredCloningMode();
                                    }
                                }));
                    }

                    state = ConnectionStatus.CONNECTED;
                }

                @Override
                public void onDisconnected() {
                    if (state == ConnectionStatus.CONNECTED) {
                        connected.forceTimeout();

                        client.disconnect();
                    }

                    state = ConnectionStatus.DISCONNECTED;
                }
            });

            ready = true;
        }
    }

    public static @NotNull CompletableFuture<Mqtt3SubAck> subscribe(String topic, Consumer<Mqtt3Publish> callback) {
        return client.subscribeWith()
                .topicFilter(topic)
                .callback(publish -> {
                    callback.accept(publish);

                    Log.d(TAG, "Message received [" + publish.getTopic() + "]: " +
                            new String(publish.getPayloadAsBytes()));
                })
                .send()
                .thenApply(mqtt3SubAck -> {
                    Log.d(TAG, "Subscribed to " + topic);

                    return null;
                });
    }

    public static @NotNull CompletableFuture<Void> unsubscribe(String topic) {
        return client.unsubscribeWith()
                .topicFilter(topic)
                .send()
                .thenApply(mqtt3SubAck -> {
                    Log.d(TAG, "Unsubscribed to " + topic);

                    return null;
                });
    }

    public static @NotNull CompletableFuture<@NotNull Mqtt3Publish> publish(String topic, byte[] bytes) {
        Mqtt3Publish message = Mqtt3Publish.builder()
                .topic(topic)
                .payload(bytes)
                .qos(MqttQos.AT_LEAST_ONCE)
                .build();

        return client.publish(message);
    }

    public static void setOnConnectionStatusChangedListener(WioMQTTClient.OnConnectionStatusChanged onConnectionStatusChangedListener) {
        connectionListener = onConnectionStatusChangedListener;

        if (connectionListener != null) {
            if (connected.getValue()) {
                connectionListener.onConnected();
            } else {
                connectionListener.onDisconnected();
            }
        }
    }

    public static void setCommandReceivedListener(CommandReceivedListener listener) {
        commandListener = listener;
    }

    public static void addTerminalModeListener(TerminalModeListener listener) {
        terminalModeListeners.add(listener);
    }

    public static void removeTerminalModeListeners() {
        terminalModeListeners.clear();
    }

    public interface OnConnectionStatusChanged {
        void onConnected();

        void onDisconnected();
    }

    public interface CommandReceivedListener {
        void onCommandReceived(Mqtt3Publish payload);
    }

    public interface TerminalModeListener {
        default void onEnteredCloningMode() {
        }

        ;

        default void onExitedCloningMode() {
        }

        ;
    }
}
