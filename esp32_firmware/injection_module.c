/**
 * @file injection_module.c
 * @brief ESP32 Injection Module Controller
 * 
 * Controls soil injection operations with precise depth and pressure monitoring.
 * Features PWM-controlled injection motor, depth/pressure sensors, and needle
 * position feedback for accurate nutrient delivery in agricultural applications.
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

// Sensor filter instances for smoothing readings
sensor_filter_t depthFilter;
sensor_filter_t pressureFilter;

// ==================== Hardware Configuration ====================
#define MOTOR_PIN          12    // Injection motor control (PWM)
#define DEPTH_SENSOR_PIN   ADC1_CHANNEL_0    // Injection depth sensor (ADC1_CH0 - GPIO36)
#define PRESSURE_PIN       ADC1_CHANNEL_3    // Injection pressure sensor (ADC1_CH3 - GPIO39)
#define NEEDLE_FEEDBACK_PIN 14   // Needle position feedback

// PWM Configuration for motor control
#define MOTOR_PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT

// ==================== Network Configuration ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT Topics for communication with central control system
const char* TOPIC_COMMAND = "exoskeleton/injection/command";
const char* TOPIC_STATUS = "exoskeleton/injection/status";
const char* TOPIC_SENSORS = "exoskeleton/injection/sensors";

// ==================== Function Declarations ====================
void app_main();
void injection_task(void *pvParameter);
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void publishSensorData();
void processCommand(cJSON* command);
void injectSoil(int targetDepth, int targetPressure);
void sendStatus(const char* state, const char* message);
void initializeHardware();

// ==================== Main Program ====================
void app_main() {
    DebugHelper::initialize();
    
    // Initialize sensor filters for noise reduction
    sensor_filter_init(&depthFilter);
    sensor_filter_init(&pressureFilter);
    
    // Initialize hardware peripherals
    initializeHardware();
    
    // Initialize MQTT helper with network credentials
    mqtt_helper_init(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT, 
                    "InjectionClient", mqttCallback);
    
    // Connect to WiFi and MQTT broker
    if (mqtt_helper_connect_wifi() && mqtt_helper_connect_broker()) {
        mqtt_helper_subscribe(TOPIC_COMMAND);
    }
    
    DebugHelper::info("Injection module initialization complete");
    sendStatus("IDLE", "System startup");
    
    // Create main task for continuous operation
    xTaskCreate(injection_task, "injection_task", 4096, NULL, 5, NULL);
}

void initializeHardware() {
    // Configure ADC for sensor readings
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(DEPTH_SENSOR_PIN, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(PRESSURE_PIN, ADC_ATTEN_DB_0);
    
    // Configure needle feedback pin as input with pullup
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << NEEDLE_FEEDBACK_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Configure injection motor PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t ledc_channel = {
        .channel = MOTOR_PWM_CHANNEL,
        .duty = 0,  // Initially stopped
        .gpio_num = MOTOR_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
}

void injection_task(void *pvParameter) {
    while (1) {
        // Process MQTT messages
        mqtt_helper_loop();
        
        static unsigned long lastUpdate = 0;
        unsigned long current_time = (unsigned long)(esp_timer_get_time() / 1000);
        if (current_time - lastUpdate > 200) { // Update sensor data at 5Hz
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
    float rawDepth = adc1_get_raw(DEPTH_SENSOR_PIN);
    float rawPressure = adc1_get_raw(PRESSURE_PIN);
    int needlePosition = gpio_get_level(NEEDLE_FEEDBACK_PIN);
    
    // Apply filtering for noise reduction
    sensor_filter_add_value(&depthFilter, rawDepth);
    sensor_filter_add_value(&pressureFilter, rawPressure);
    
    // Calibrate sensor readings to real-world units
    float calibratedDepth = calibrate_pressure(sensor_filter_get_filtered(&depthFilter));  // Reuse pressure calibration
    float calibratedPressure = calibrate_pressure(sensor_filter_get_filtered(&pressureFilter));
    
    // Add sensor data to JSON
    cJSON_AddNumberToObject(json, "depth", calibratedDepth);
    cJSON_AddNumberToObject(json, "pressure", calibratedPressure);
    cJSON_AddBoolToObject(json, "needle_position", needlePosition);
    
    // Serialize and publish sensor data
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_SENSORS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// ==================== Command Processing ====================
void processCommand(cJSON* command) {
    cJSON* action = cJSON_GetObjectItem(command, "action");
    
    if (cJSON_IsString(action) && strcmp(action->valuestring, "inject") == 0) {
        cJSON* params = cJSON_GetObjectItem(command, "params");
        cJSON* depth = cJSON_GetObjectItem(params, "depth");
        cJSON* pressure = cJSON_GetObjectItem(params, "pressure");
        
        if (cJSON_IsNumber(depth) && cJSON_IsNumber(pressure)) {
            injectSoil(depth->valueint, pressure->valueint);
        }
    }
    else if (cJSON_IsString(action) && strcmp(action->valuestring, "retract") == 0) {
        // Stop motor and retract needle
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
        sendStatus("RETRACTING", "Retracting needle");
        DebugHelper::info("Needle retraction initiated");
    }
}

// ==================== Injection Control ====================
void injectSoil(int targetDepth, int targetPressure) {
    DebugHelper::info("Starting soil injection - Target depth: %d, Target pressure: %d", 
                     targetDepth, targetPressure);
    sendStatus("INJECTING", "Starting injection...");
    
    // Calculate motor power based on target pressure (PWM control)
    int motorPower = (targetPressure * 255) / 300;  // Map 0-300 to 0-255 PWM range
    if (motorPower > 255) motorPower = 255;  // Clamp to maximum
    if (motorPower < 150) motorPower = 150;  // Minimum power for operation
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, motorPower);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
    
    // Monitor injection progress with timeout protection
    unsigned long startTime = (unsigned long)(esp_timer_get_time() / 1000);
    while (true) {
        int currentDepth = adc1_get_raw(DEPTH_SENSOR_PIN);
        
        // Check if target depth reached
        if (currentDepth >= targetDepth) {
            DebugHelper::info("Target depth reached: %d", currentDepth);
            break;
        }
        
        // Timeout protection - prevent infinite operation
        if ((unsigned long)(esp_timer_get_time() / 1000) - startTime > 10000) { // 10 second timeout
            DebugHelper::error("Injection timeout - operation aborted");
            sendStatus("ERROR", "Injection timeout");
            break;
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);  // 50ms monitoring interval
    }
    
    // Stop injection motor
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
    
    DebugHelper::info("Injection operation completed");
    sendStatus("COMPLETED", "Injection completed");
}

// ==================== Status Reporting ====================
void sendStatus(const char* state, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "module", "injection");
    cJSON_AddStringToObject(json, "state", state);
    cJSON_AddStringToObject(json, "message", message);
    cJSON_AddNumberToObject(json, "timestamp", (unsigned long)(esp_timer_get_time() / 1000));
    
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_STATUS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    DebugHelper::info("Status report: %s - %s", state, message);
}
