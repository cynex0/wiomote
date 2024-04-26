package se.gu.wiomote.network;

import android.util.Log;

import com.hivemq.client.mqtt.MqttGlobalPublishFilter;
import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.mqtt3.Mqtt3AsyncClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3BlockingClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3Client;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;

import java.util.UUID;

public class WioMqttClient {
    final String MQTT_LOGTAG = "se.gu.wiomote.MQTT";
    final String IN_TOPIC = "wiomote/connection/terminal";
    final String OUT_TOPIC = "wiomote/connection/app";
    final String HOST = "broker.hivemq.com";
    final int PORT = 1883;
    final Mqtt3AsyncClient asyncClient;
    final Mqtt3BlockingClient blockingClient;

    // Build the client
    public WioMqttClient() {
        final Mqtt3Client genericClient = Mqtt3Client.builder()
                .identifier("WIOmote_app-" + UUID.randomUUID())
                .serverHost(HOST)
                .serverPort(PORT)
                .build();
        asyncClient = genericClient.toAsync();
        blockingClient = genericClient.toBlocking();

        // Register a callback for received messages
        asyncClient.publishes(MqttGlobalPublishFilter.ALL, publish -> {
            String message = new String(publish.getPayloadAsBytes());
            String topic = publish.getTopic().toString();
            Log.i(MQTT_LOGTAG, "Message received [" + topic + "]: " + message);

            // If a ping is received, send back a pong
            if (topic.equals(IN_TOPIC) && message.equals("ping")) {
                publish("pong");
            }
        }); // registering a callback before connecting ensures no messages get lost during connection

        connect();
    }

    public void connect() {
        asyncClient.connect() // connect. returns a future that will be completed once an acknowledgement is received
                .thenCompose(connAck -> { // attach handling
                    Log.i(MQTT_LOGTAG, "Successfully connected to broker: " + HOST + ":" + PORT);
                    return asyncClient.subscribeWith().topicFilter(IN_TOPIC).send(); // subscribe
                }).thenCompose(subAck -> {
                    Log.i(MQTT_LOGTAG, "Subscribed to " + IN_TOPIC);
                    final Mqtt3Publish message = Mqtt3Publish.builder() // build a message
                            .topic(OUT_TOPIC)
                            .payload(("App connected!").getBytes())
                            .qos(MqttQos.AT_LEAST_ONCE)
                            .build();
                    return asyncClient.publish(message); // return a publish future
                }).thenApply(pubAck -> {
                    return Log.i(MQTT_LOGTAG, String.format("Message published [%s]: %s", pubAck.getTopic(), new String(pubAck.getPayloadAsBytes())));
                }).exceptionally(throwable -> { // attach exception handling
                    System.out.println("Something went wrong!");
                    return null;
                });
    }

    public void subscribe(String topic) {
        blockingClient.subscribeWith()
                .topicFilter(topic)
                .send();
        Log.i(MQTT_LOGTAG, "Subscribed to " + topic);
    }

    public void publish(String topic, String content) {
        final Mqtt3Publish message = Mqtt3Publish.builder() // build a message
                .topic(topic)
                .payload((content).getBytes())
                .qos(MqttQos.AT_LEAST_ONCE)
                .build();
        blockingClient.publish(message);
        Log.i(MQTT_LOGTAG, String.format("Message published [%s]: %s", topic, content));
    }

    public void publish(String content) {
        publish(OUT_TOPIC, content);
    }
}
