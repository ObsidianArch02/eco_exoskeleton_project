/**
 * @file greenhouse_module.c
 * @brief ESP32 Greenhouse Module Controller
 * 
 * Controls deployment and retraction of foldable greenhouse structures with
 * environmental monitoring. Features dual-state actuator control, position
 * feedback sensors, and temperature/humidity monitoring for agricultural
 * protection systems.
 */

#include "mqtt_helper.h"
#include "debug_helper.h"
#include "sensor_filter_c.h"
#include "sensor_calibration.h"
#include <cJSON.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Sensor filter instances for environmental monitoring
sensor_filter_t tempFilter;
sensor_filter_t humidityFilter;

// ==================== Hardware Configuration ====================
#define DEPLOY_PIN         12    // Greenhouse deployment control pin
#define RETRACT_PIN        13    // Greenhouse retraction control pin
#define DEPLOY_FEEDBACK_PIN 14   // Deployment position feedback sensor
#define RETRACT_FEEDBACK_PIN 15  // Retraction position feedback sensor
#define TEMP_SENSOR_PIN    ADC1_CHANNEL_0    // Temperature sensor (ADC1_CH0 - GPIO36)
#define HUMIDITY_PIN       ADC1_CHANNEL_3    // Humidity sensor (ADC1_CH3 - GPIO39)

// ==================== Network Configuration ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT Topics for communication with central control system
const char* TOPIC_COMMAND = "exoskeleton/greenhouse/command";
const char* TOPIC_STATUS = "exoskeleton/greenhouse/status";
const char* TOPIC_SENSORS = "exoskeleton/greenhouse/sensors";

// ==================== Function Declarations ====================
void app_main();
void greenhouse_task(void *pvParameter);
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void publishSensorData();
void processCommand(cJSON* command);
void deployGreenhouse();
void retractGreenhouse();
void sendStatus(const char* state, const char* message);
void initializeHardware();

// ==================== Main Program ====================
void app_main() {
    DebugHelper::initialize();
    
    // Initialize sensor filters for environmental monitoring
    sensor_filter_init(&tempFilter);
    sensor_filter_init(&humidityFilter);
    
    // Initialize hardware peripherals
    initializeHardware();
    
    // Initialize MQTT helper with network credentials
    mqtt_helper_init(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT, 
                    "ESP32_Greenhouse", mqttCallback);
    
    // Connect to WiFi and MQTT broker
    if (mqtt_helper_connect_wifi() && mqtt_helper_connect_broker()) {
        mqtt_helper_subscribe(TOPIC_COMMAND);
    }
    
    DebugHelper::info("Greenhouse module initialization complete");
    sendStatus("IDLE", "System startup");
    
    // Create main task for continuous operation
    xTaskCreate(greenhouse_task, "greenhouse_task", 4096, NULL, 5, NULL);
}

void initializeHardware() {
    // Configure deployment control pins as outputs
    gpio_config_t output_conf = {
        .pin_bit_mask = (1ULL << DEPLOY_PIN) | (1ULL << RETRACT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&output_conf);
    
    // Configure feedback pins as inputs with pullup
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << DEPLOY_FEEDBACK_PIN) | (1ULL << RETRACT_FEEDBACK_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&input_conf);
    
    // Set initial state - both actuators off
    gpio_set_level(DEPLOY_PIN, 0);
    gpio_set_level(RETRACT_PIN, 0);
    
    // Configure ADC for environmental sensors
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(TEMP_SENSOR_PIN, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(HUMIDITY_PIN, ADC_ATTEN_DB_0);
}

void greenhouse_task(void *pvParameter) {
    while (1) {
        // Process MQTT messages
        mqtt_helper_loop();
        
        static unsigned long lastUpdate = 0;
        unsigned long current_time = (unsigned long)(esp_timer_get_time() / 1000);
        if (current_time - lastUpdate > 1000) { // Update sensor data every second
            publishSensorData();
            lastUpdate = current_time;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ==================== MQTT Callback ====================
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    DebugHelper::info("Received message [%s]", topic);
    
    // Handle internal resubscription message for connection recovery
    if (strcmp(topic, "internal/resubscribe") == 0) {
        DebugHelper::info("Resubscribing to topic: %s", TOPIC_COMMAND);
        mqtt_helper_subscribe(TOPIC_COMMAND);
        return;
    }
    
    // Create null-terminated string from payload
    char* json_string = (char*)malloc(length + 1);
    memcpy(json_string, payload, length);
    json_string[length] = '\0';
    
    // Parse JSON command
    cJSON* json = cJSON_Parse(json_string);
    free(json_string);
    
    if (json == NULL) {
        DebugHelper::error("JSON parsing failed");
        return;
    }
    
    // Process command
    if (strcmp(topic, TOPIC_COMMAND) == 0) {
        processCommand(json);
    }
    
    cJSON_Delete(json);
}

// ==================== Sensor Data Publishing ====================
void publishSensorData() {
    // Create JSON object for environmental readings
    cJSON* json = cJSON_CreateObject();
    
    // Read raw sensor values
    float rawTemp = adc1_get_raw(TEMP_SENSOR_PIN);
    float rawHumidity = adc1_get_raw(HUMIDITY_PIN);
    
    // Apply filtering for stable readings
    sensor_filter_add_value(&tempFilter, rawTemp);
    sensor_filter_add_value(&humidityFilter, rawHumidity);
    
    // Calibrate sensor readings to real-world units
    float calibratedTemp = calibrate_temperature(sensor_filter_get_filtered(&tempFilter));
    float calibratedHumidity = (sensor_filter_get_filtered(&humidityFilter) / 4095.0) * 100.0;  // Convert to percentage
    
    // Read position feedback sensors
    bool isDeployed = gpio_get_level(DEPLOY_FEEDBACK_PIN) == 1;
    bool isRetracted = gpio_get_level(RETRACT_FEEDBACK_PIN) == 1;
    
    // Add sensor data to JSON
    cJSON_AddNumberToObject(json, "temperature", calibratedTemp);
    cJSON_AddNumberToObject(json, "humidity", calibratedHumidity);
    cJSON_AddBoolToObject(json, "deployed", isDeployed);
    cJSON_AddBoolToObject(json, "retracted", isRetracted);
    
    // Serialize and publish sensor data
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_SENSORS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    DebugHelper::info("Greenhouse sensor data published");
}

// ==================== Command Processing ====================
void processCommand(cJSON* command) {
    cJSON* action = cJSON_GetObjectItem(command, "action");
    
    DebugHelper::info("Executing command: %s", cJSON_GetStringValue(action));
    
    if (cJSON_IsString(action) && strcmp(action->valuestring, "deploy") == 0) {
        deployGreenhouse();
    }
    else if (cJSON_IsString(action) && strcmp(action->valuestring, "retract") == 0) {
        retractGreenhouse();
    }
    else {
        DebugHelper::warning("Unknown command: %s", cJSON_GetStringValue(action));
    }
}

// ==================== Greenhouse Control ====================
void deployGreenhouse() {
    sendStatus("DEPLOYING", "Deploying greenhouse...");
    
    // Activate deployment mechanism
    gpio_set_level(DEPLOY_PIN, 1);
    
    // Wait for deployment completion signal with timeout
    unsigned long startTime = (unsigned long)(esp_timer_get_time() / 1000);
    while (gpio_get_level(DEPLOY_FEEDBACK_PIN) == 0) {
        if ((unsigned long)(esp_timer_get_time() / 1000) - startTime > 5000) { // 5 second timeout
            sendStatus("ERROR", "Greenhouse deployment timeout");
            gpio_set_level(DEPLOY_PIN, 0);
            DebugHelper::error("Deployment timeout - operation aborted");
            return;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    // Stop deployment mechanism
    gpio_set_level(DEPLOY_PIN, 0);
    DebugHelper::info("Greenhouse deployment completed successfully");
    sendStatus("DEPLOYED", "Greenhouse deployment complete");
}

void retractGreenhouse() {
    sendStatus("RETRACTING", "Retracting greenhouse...");
    
    // Activate retraction mechanism
    gpio_set_level(RETRACT_PIN, 1);
    
    // Wait for retraction completion signal with timeout
    unsigned long startTime = (unsigned long)(esp_timer_get_time() / 1000);
    while (gpio_get_level(RETRACT_FEEDBACK_PIN) == 0) {
        if ((unsigned long)(esp_timer_get_time() / 1000) - startTime > 5000) { // 5 second timeout
            sendStatus("ERROR", "Greenhouse retraction timeout");
            gpio_set_level(RETRACT_PIN, 0);
            DebugHelper::error("Retraction timeout - operation aborted");
            return;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    // Stop retraction mechanism
    gpio_set_level(RETRACT_PIN, 0);
    DebugHelper::info("Greenhouse retraction completed successfully");
    sendStatus("RETRACTED", "Greenhouse retraction complete");
}

// ==================== Status Reporting ====================
void sendStatus(const char* state, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "module", "greenhouse");
    cJSON_AddStringToObject(json, "state", state);
    cJSON_AddStringToObject(json, "message", message);
    cJSON_AddNumberToObject(json, "timestamp", (unsigned long)(esp_timer_get_time() / 1000));
    
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_STATUS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    DebugHelper::info("Status report: %s - %s", state, message);
}
