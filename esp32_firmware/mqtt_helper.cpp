#include "mqtt_helper.h"
#include "debug_helper.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Configuration variables
const char* wifi_ssid = nullptr;
const char* wifi_password = nullptr;
const char* mqtt_server = nullptr;
int mqtt_port = 1883;
const char* client_id = nullptr;
void (*user_callback)(char*, uint8_t*, unsigned int) = nullptr;

void mqtt_helper_init(const char* ssid, const char* password, 
                      const char* server, int port, 
                      const char* id,
                      void (*callback)(char*, uint8_t*, unsigned int)) {
    wifi_ssid = ssid;
    wifi_password = password;
    mqtt_server = server;
    mqtt_port = port;
    client_id = id;
    user_callback = callback;
    
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(user_callback);
    
    // Initialize debug helper
    DebugHelper::initialize();
}

bool mqtt_helper_connect_wifi() {
    WiFi.begin(wifi_ssid, wifi_password);
    DebugHelper::info("Connecting to WiFi: %s", wifi_ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DebugHelper::verbose(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        DebugHelper::info("WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
        return true;
    } else {
        DebugHelper::error("WiFi connection failed");
        return false;
    }
}

bool mqtt_helper_connect_broker() {
    if (!mqttClient.connected()) {
        DebugHelper::info("Connecting to MQTT broker: %s:%d", mqtt_server, mqtt_port);
        if (mqttClient.connect(client_id)) {
            DebugHelper::info("MQTT connected!");
            return true;
        } else {
            DebugHelper::error("MQTT connection failed, state: %d", mqttClient.state());
            return false;
        }
    }
    return true;
}

void mqtt_helper_reconnect() {
    while (!mqttClient.connected()) {
        DebugHelper::warning("MQTT connection lost. Reconnecting...");
        
        // First ensure WiFi is connected
        if (WiFi.status() != WL_CONNECTED) {
            DebugHelper::info("WiFi disconnected, reconnecting...");
            mqtt_helper_connect_wifi();
        }
        
        // Then connect to MQTT
        if (mqtt_helper_connect_broker()) {
            DebugHelper::info("Re-subscribing to topics");
        } else {
            DebugHelper::info("Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void mqtt_helper_loop() {
    if (!mqttClient.connected()) {
        mqtt_helper_reconnect();
    }
    mqttClient.loop();
}

bool mqtt_helper_publish(const char* topic, const char* payload) {
    return mqttClient.publish(topic, payload);
}

bool mqtt_helper_subscribe(const char* topic) {
    return mqttClient.subscribe(topic);
}