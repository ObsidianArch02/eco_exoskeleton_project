#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include <WiFi.h>
#include <PubSubClient.h>

/**
 * @brief Initialize MQTT helper
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @param server MQTT broker address
 * @param port MQTT broker port
 * @param id Client ID
 * @param callback Message callback function
 */
void mqtt_helper_init(const char* ssid, 
                     const char* password,
                     const char* server, 
                     int port,
                     const char* id,
                     void (*callback)(char*, uint8_t*, unsigned int));

/**
 * @brief Connect to WiFi
 * @return true if connected successfully
 */
bool mqtt_helper_connect_wifi();

/**
 * @brief Connect to MQTT broker
 * @return true if connected successfully
 */
bool mqtt_helper_connect_broker();

/**
 * @brief Maintain MQTT connection
 */
void mqtt_helper_reconnect();

/**
 * @brief MQTT main loop
 */
void mqtt_helper_loop();

/**
 * @brief Publish MQTT message
 * @param topic Topic to publish to
 * @param payload Message payload
 * @return true if published successfully
 */
bool mqtt_helper_publish(const char* topic, const char* payload);

/**
 * @brief Subscribe to MQTT topic
 * @param topic Topic to subscribe
 * @return true if subscribed successfully
 */
bool mqtt_helper_subscribe(const char* topic);

#endif // MQTT_HELPER_H