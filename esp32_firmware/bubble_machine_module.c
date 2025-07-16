/**
 * @file bubble_machine_module.c
 * @brief ESP32 Bubble Machine Module Controller
 * 
 * Controls repair solution spraying with adjustable intensity and flow monitoring.
 * Features PWM-controlled spray nozzle, flow rate monitoring, tank level sensing,
 * and system pressure monitoring for precise repair solution application.
 */

#include "mqtt_helper.h"
#include "debug_helper.h"
#include "sensor_filter_c.h"
#include "sensor_calibration.h"
#include <cJSON.h>
#include <driver/ledc.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Sensor filter instances for noise reduction
sensor_filter_t pressureFilter;
sensor_filter_t flowFilter;
sensor_filter_t tankLevelFilter;

// ==================== Hardware Configuration ====================
#define NOZZLE_PIN         12    // Spray nozzle control pin
#define FLOW_SENSOR_PIN    ADC1_CHANNEL_0    // Flow rate sensor (ADC1_CH0 - GPIO36)
#define TANK_LEVEL_PIN     ADC1_CHANNEL_3    // Tank level sensor (ADC1_CH3 - GPIO39)
#define PRESSURE_PIN       14    // System pressure sensor

// PWM Configuration for nozzle control
#define NOZZLE_PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT

// ==================== Network Configuration ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT Topics for communication with central control system
const char* TOPIC_COMMAND = "exoskeleton/bubble/command";
const char* TOPIC_STATUS = "exoskeleton/bubble/status";
const char* TOPIC_SENSORS = "exoskeleton/bubble/sensors";

// ==================== Function Declarations ====================
void app_main();
void bubble_task(void *pvParameter);
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void publishSensorData();
void processCommand(cJSON* command);
void sprayBubbles(int duration, int intensity);
void sendStatus(const char* state, const char* message);
void initializeHardware();

// ==================== Main Program ====================
void app_main() {
    DebugHelper::initialize();
    
    // Initialize sensor filters for noise reduction
    sensor_filter_init(&pressureFilter);
    sensor_filter_init(&flowFilter);
    sensor_filter_init(&tankLevelFilter);
    
    // Initialize hardware peripherals
    initializeHardware();
    
    // Initialize MQTT helper with network credentials
    mqtt_helper_init(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT, 
                    "BubbleMachineClient", mqttCallback);
    
    // Connect to WiFi and MQTT broker
    if (mqtt_helper_connect_wifi() && mqtt_helper_connect_broker()) {
        mqtt_helper_subscribe(TOPIC_COMMAND);
    }
    
    DebugHelper::info("Bubble machine module initialization complete");
    sendStatus("IDLE", "System startup");
    
    // Create main task for continuous operation
    xTaskCreate(bubble_task, "bubble_task", 4096, NULL, 5, NULL);
}

void initializeHardware() {
    // Configure ADC for sensor readings
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(FLOW_SENSOR_PIN, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(TANK_LEVEL_PIN, ADC_ATTEN_DB_0);
    
    // Configure pressure sensor GPIO as input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PRESSURE_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Configure spray nozzle PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t ledc_channel = {
        .channel = NOZZLE_PWM_CHANNEL,
        .duty = 0,
        .gpio_num = NOZZLE_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
}

void bubble_task(void *pvParameter) {
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
    // Create JSON object for sensor readings
    cJSON* json = cJSON_CreateObject();
    
    // Read raw sensor values
    float rawPressure = gpio_get_level(PRESSURE_PIN);
    float rawFlow = adc1_get_raw(FLOW_SENSOR_PIN);
    float rawTank = adc1_get_raw(TANK_LEVEL_PIN);
    
    // Apply filtering for noise reduction
    sensor_filter_add_value(&pressureFilter, rawPressure);
    sensor_filter_add_value(&flowFilter, rawFlow);
    sensor_filter_add_value(&tankLevelFilter, rawTank);
    
    // Calibrate sensor readings to real-world units
    float calibratedPressure = calibrate_pressure(sensor_filter_get_filtered(&pressureFilter));
    float calibratedFlow = calibrate_flow(sensor_filter_get_filtered(&flowFilter));
    float calibratedTank = calibrate_flow(sensor_filter_get_filtered(&tankLevelFilter));
    
    // Add sensor data to JSON
    cJSON_AddNumberToObject(json, "flow_rate", calibratedFlow);
    cJSON_AddNumberToObject(json, "tank_level", calibratedTank);
    cJSON_AddNumberToObject(json, "system_pressure", calibratedPressure);
    
    // Serialize and publish sensor data
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_SENSORS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// ==================== Command Processing ====================
void processCommand(cJSON* command) {
    cJSON* action = cJSON_GetObjectItem(command, "action");
    
    if (cJSON_IsString(action) && strcmp(action->valuestring, "spray") == 0) {
        cJSON* params = cJSON_GetObjectItem(command, "params");
        cJSON* duration = cJSON_GetObjectItem(params, "duration");
        cJSON* intensity = cJSON_GetObjectItem(params, "intensity");
        
        if (cJSON_IsNumber(duration) && cJSON_IsNumber(intensity)) {
            sprayBubbles(duration->valueint, intensity->valueint);
        }
    }
}

// ==================== Spray Control ====================
void sprayBubbles(int duration, int intensity) {
    DebugHelper::info("Spraying repair solution - Duration: %dms, Intensity: %d%%", duration, intensity);
    sendStatus("SPRAYING", "Spraying repair solution...");
    
    // Adjust nozzle intensity based on target (PWM control)
    int pwmValue = (intensity * 255) / 100;  // Map 0-100% to 0-255 PWM range
    ledc_set_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL, pwmValue);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL);
    
    // Record start time for duration monitoring
    unsigned long startTime = (unsigned long)(esp_timer_get_time() / 1000);
    
    // Monitor spraying process with safety checks
    while ((unsigned long)(esp_timer_get_time() / 1000) - startTime < duration) {
        // Check if system pressure is adequate
        int pressure = gpio_get_level(PRESSURE_PIN);
        if (pressure == 0) { // Pressure too low
            DebugHelper::error("Insufficient system pressure: %d", pressure);
            sendStatus("ERROR", "Insufficient system pressure");
            break;
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    // Stop spraying - turn off nozzle
    ledc_set_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL);
    DebugHelper::info("Spraying operation completed");
    sendStatus("COMPLETED", "Spraying completed");
}

// ==================== Status Reporting ====================
void sendStatus(const char* state, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "module", "bubble");
    cJSON_AddStringToObject(json, "state", state);
    cJSON_AddStringToObject(json, "message", message);
    cJSON_AddNumberToObject(json, "timestamp", (unsigned long)(esp_timer_get_time() / 1000));
    
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_STATUS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    DebugHelper::info("Status report: %s - %s", state, message);
}
