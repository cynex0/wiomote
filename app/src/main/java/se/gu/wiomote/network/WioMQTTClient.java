package se.gu.wiomote.network;

import android.util.Log;

import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.mqtt3.Mqtt3AsyncClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3Client;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;
import com.hivemq.client.mqtt.mqtt3.message.subscribe.suback.Mqtt3SubAck;

import org.jetbrains.annotations.NotNull;

import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;

public class WioMQTTClient {
    private static final String TAG = "se.gu.wiomote.MQTT";
    private static final String ID_SUFFIX = "WIOmote_app-";
    private static final String HOST = "broker.hivemq.com";
    private static final int PORT = 1883;
    private static final String CONN_IN_TOPIC = "wiomote/connection/terminal";
    private static final String CONN_OUT_TOPIC = "wiomote/connection/app";
    private static final Mqtt3AsyncClient client = Mqtt3Client.builder()
            .identifier(ID_SUFFIX + UUID.randomUUID())
            .serverHost(HOST)
            .serverPort(PORT)
            .build()
            .toAsync();
    private static boolean ready = false;

    private static void prepareConnection() {
        if (!ready) {
            WiFiHandler.addOnNetworkChangedListener(new WiFiHandler.OnNetworkChanged() {
                @Override
                public void onConnected() {
                    client.connect()
                            .thenCompose(connAck -> { // attach handling
                                Log.d(TAG, "Successfully connected to broker - " + HOST + ":" + PORT);

                                return subscribe(CONN_IN_TOPIC, payload -> {
                                    //TODO handle connection in callback
                                });
                            }).thenCompose(subAck -> {
                                return publish(CONN_OUT_TOPIC, "App connected!".getBytes()); // return a publish future
                            });

                    WiFiHandler.removeOnNetworkChangedListener(this);
                }
            });

            ready = true;
        }
    }

    public static @NotNull CompletableFuture<Mqtt3SubAck> subscribe(String topic, Consumer<Mqtt3Publish> callback) {
        prepareConnection();

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
        prepareConnection();

        Mqtt3Publish message = Mqtt3Publish.builder()
                .topic(topic)
                .payload(bytes)
                .qos(MqttQos.AT_LEAST_ONCE)
                .build();

        Log.i(TAG, "publish[" + topic + "]: " + new String(bytes));
        return client.publish(message);
    }
}
