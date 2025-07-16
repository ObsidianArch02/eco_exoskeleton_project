#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file mqtt_helper.h
 * @brief MQTT communication helper for ESP32 modules
 * 
 * This helper provides a simplified interface for MQTT communication using 
 * ESP-IDF native WiFi and MQTT client libraries. It handles WiFi connection,
 * MQTT broker connection, message publishing/subscribing, and automatic
 * reconnection with retry logic.
 */

/**
 * @brief Initialize MQTT helper with network and broker configuration
 * 
 * Sets up WiFi credentials, MQTT broker details, and message callback.
 * This function must be called before any other MQTT operations.
 * 
 * @param ssid WiFi network SSID to connect to
 * @param password WiFi network password
 * @param server MQTT broker IP address or hostname
 * @param port MQTT broker port (typically 1883 for non-TLS)
 * @param id Unique client ID for MQTT connection
 * @param callback Function to handle incoming MQTT messages
 *                 Signature: void callback(char* topic, uint8_t* payload, unsigned int length)
 */
void mqtt_helper_init(const char* ssid, 
                     const char* password,
                     const char* server, 
                     int port,
                     const char* id,
                     void (*callback)(char*, uint8_t*, unsigned int));

/**
 * @brief Connect to WiFi network
 * 
 * Attempts to connect to the WiFi network using credentials provided
 * in mqtt_helper_init(). Will retry up to 20 times with 500ms delays.
 * 
 * @return true if WiFi connection successful, false if failed after retries
 */
bool mqtt_helper_connect_wifi();

/**
 * @brief Connect to MQTT broker
 * 
 * Establishes connection to the MQTT broker using settings from 
 * mqtt_helper_init(). Sets up event handlers for connection management.
 * 
 * @return true if MQTT connection successful, false if failed
 */
bool mqtt_helper_connect_broker();

/**
 * @brief Handle MQTT reconnection logic
 * 
 * Called automatically when connection is lost. Attempts to reconnect
 * with exponential backoff. Will restart the device after 5 failed attempts.
 * Also handles WiFi reconnection if needed.
 */
void mqtt_helper_reconnect();

/**
 * @brief MQTT main loop for message processing
 * 
 * Should be called regularly (e.g., in main task loop) to process
 * incoming messages and maintain connection. Automatically triggers
 * reconnection if connection is lost.
 */
void mqtt_helper_loop();

/**
 * @brief Publish message to MQTT topic
 * 
 * Sends a message to the specified MQTT topic. Message is published
 * with QoS 1 (at least once delivery) and retain flag set to false.
 * 
 * @param topic MQTT topic to publish to (null-terminated string)
 * @param payload Message payload (null-terminated string)
 * @return true if message was queued for transmission, false if failed
 */
bool mqtt_helper_publish(const char* topic, const char* payload);

/**
 * @brief Subscribe to MQTT topic
 * 
 * Subscribes to the specified topic to receive messages. Incoming messages
 * will be delivered to the callback function set in mqtt_helper_init().
 * 
 * @param topic MQTT topic to subscribe to (supports wildcards + and #)
 * @return true if subscription request was sent, false if failed
 */
bool mqtt_helper_subscribe(const char* topic);

#endif // MQTT_HELPER_H