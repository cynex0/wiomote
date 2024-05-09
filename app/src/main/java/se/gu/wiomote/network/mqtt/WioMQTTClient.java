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

import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;

import se.gu.wiomote.network.WiFiHandler;
import se.gu.wiomote.utils.TimeoutBoolean;
import se.gu.wiomote.utils.Utils;

public class WioMQTTClient {
    private static final String TAG = "se.gu.wiomote.MQTT";
    private static final String ID_SUFFIX = "WIOmote_app-";
    private static final String HOST = "broker.hivemq.com";
    private static final int PORT = 1883;
    private static final String CONN_IN_TOPIC = "wiomote/connection/terminal";
    private static final String CONN_OUT_TOPIC = "wiomote/connection/app";
    private static final TimeoutBoolean connected = new TimeoutBoolean(6900);
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
    private static WioMQTTClient.OnConnectionStatusChanged listener = null;
    private static ConnectionStatus state = ConnectionStatus.UNKNOWN;
    private static boolean ready = false;

    public static void prepare() {
        if (!ready) {
            connected.setOnUpdateListener(value ->
                    Utils.runOnUiThread(() -> {
                        if (listener != null) {
                            if (value) {
                                listener.onConnected();
                            } else {
                                listener.onDisconnected();
                            }
                        }
                    }));

            WiFiHandler.addOnNetworkChangedListener(new WiFiHandler.OnNetworkChanged() {

                @Override
                public void onConnected() {
                    if (state == ConnectionStatus.UNKNOWN) {
                        client.connect()
                                .thenCompose(connAck -> { // attach handling
                                    Log.d(TAG, "Successfully connected to broker - " + HOST + ":" + PORT);

                                    return subscribe(CONN_IN_TOPIC, payload -> connected.setTrue());
                                }).thenCompose(subAck -> {
                                    return publish(CONN_OUT_TOPIC, "App connected!".getBytes()); // return a publish future
                                });
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
        listener = onConnectionStatusChangedListener;

        if (listener != null) {
            if (connected.getValue()) {
                listener.onConnected();
            } else {
                listener.onDisconnected();
            }
        }
    }

    public interface OnConnectionStatusChanged {
        void onConnected();

        void onDisconnected();
    }
}
