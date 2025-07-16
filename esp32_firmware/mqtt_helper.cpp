#include "mqtt_helper.h"
#include "debug_helper.h"
#include <string.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <mqtt_client.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

static const char* TAG = "mqtt_helper";

// WiFi event group to signal connection status
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Configuration variables
static const char* wifi_ssid = nullptr;
static const char* wifi_password = nullptr;
static const char* mqtt_server = nullptr;
static int mqtt_port = 1883;
static const char* client_id = nullptr;
static void (*user_callback)(char*, uint8_t*, unsigned int) = nullptr;

static esp_mqtt_client_handle_t mqtt_client = nullptr;
static bool mqtt_connected = false;
static int wifi_retry_num = 0;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_num < 20) {
            esp_wifi_connect();
            wifi_retry_num++;
            DebugHelper::info("retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        DebugHelper::info("connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        DebugHelper::info("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// MQTT event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            DebugHelper::info("MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            DebugHelper::info("MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            DebugHelper::info("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            DebugHelper::info("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            DebugHelper::info("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            DebugHelper::info("MQTT_EVENT_DATA");
            if (user_callback) {
                // Create null-terminated topic string
                char topic[event->topic_len + 1];
                memcpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';
                
                user_callback(topic, (uint8_t*)event->data, event->data_len);
            }
            break;
        case MQTT_EVENT_ERROR:
            DebugHelper::error("MQTT_EVENT_ERROR");
            break;
        default:
            DebugHelper::info("Other event id:%d", event->event_id);
            break;
    }
}

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
    
    // Initialize debug helper
    DebugHelper::initialize();
    
    // Initialize TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create WiFi event group
    s_wifi_event_group = xEventGroupCreate();
    
    DebugHelper::info("MQTT Helper initialized");
}

bool mqtt_helper_connect_wifi() {
    // Create default WiFi STA interface
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, wifi_ssid);
    strcpy((char*)wifi_config.sta.password, wifi_password);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    DebugHelper::info("Connecting to WiFi: %s", wifi_ssid);
    
    // Wait for connection or failure
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    
    if (bits & WIFI_CONNECTED_BIT) {
        DebugHelper::info("WiFi connected successfully");
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        DebugHelper::error("WiFi connection failed");
        return false;
    }
    
    return false;
}

bool mqtt_helper_connect_broker() {
    if (mqtt_connected) {
        return true;
    }
    
    char mqtt_uri[128];
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtt://%s:%d", mqtt_server, mqtt_port);
    
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.uri = mqtt_uri;
    mqtt_cfg.client_id = client_id;
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == nullptr) {
        DebugHelper::error("Failed to initialize MQTT client");
        return false;
    }
    
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
    
    DebugHelper::info("Connecting to MQTT broker: %s:%d", mqtt_server, mqtt_port);
    
    // Wait for connection (with timeout)
    int timeout_count = 0;
    while (!mqtt_connected && timeout_count < 50) { // 5 second timeout
        vTaskDelay(100 / portTICK_PERIOD_MS);
        timeout_count++;
    }
    
    if (mqtt_connected) {
        DebugHelper::info("MQTT connected!");
        return true;
    } else {
        DebugHelper::error("MQTT connection timeout");
        return false;
    }
}

void mqtt_helper_reconnect() {
    static int reconnectAttempts = 0;
    
    while (!mqtt_connected) {
        DebugHelper::warning("MQTT connection lost. Reconnecting... (Attempt %d)", reconnectAttempts+1);
        
        if (mqtt_helper_connect_broker()) {
            DebugHelper::info("Re-subscribing to topics");
            // Notify modules to resubscribe
            if (user_callback) {
                user_callback((char*)"internal/resubscribe", nullptr, 0);
            }
            reconnectAttempts = 0;
        } else {
            reconnectAttempts++;
            DebugHelper::warning("Retrying in 5 seconds...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            
            // Reset after 5 attempts
            if (reconnectAttempts >= 5) {
                DebugHelper::error("Resetting after 5 failed attempts");
                esp_restart();
            }
        }
    }
}

void mqtt_helper_loop() {
    if (!mqtt_connected) {
        mqtt_helper_reconnect();
    }
    // ESP-IDF MQTT client handles message processing automatically
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

bool mqtt_helper_publish(const char* topic, const char* payload) {
    if (!mqtt_connected || mqtt_client == nullptr) {
        return false;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
    return msg_id != -1;
}

bool mqtt_helper_subscribe(const char* topic) {
    if (!mqtt_connected || mqtt_client == nullptr) {
        return false;
    }
    
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
    return msg_id != -1;
}