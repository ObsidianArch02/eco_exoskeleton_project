/**
 * ESP32 泡泡机模块控制器
 * 负责修复液喷洒控制
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

// 传感器过滤器实例
sensor_filter_t pressureFilter;
sensor_filter_t flowFilter;
sensor_filter_t tankLevelFilter;

// ==================== 硬件配置 ====================
#define NOZZLE_PIN         12    // 喷嘴控制引脚
#define FLOW_SENSOR_PIN    ADC1_CHANNEL_0    // 流量传感器 (ADC1_CH0 - GPIO36)
#define TANK_LEVEL_PIN     ADC1_CHANNEL_3    // 储液罐液位传感器 (ADC1_CH3 - GPIO39)
#define PRESSURE_PIN       14    // 系统压力传感器

// PWM配置
#define NOZZLE_PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT

// ==================== 网络配置 ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT 主题
const char* TOPIC_COMMAND = "exoskeleton/bubble/command";
const char* TOPIC_STATUS = "exoskeleton/bubble/status";
const char* TOPIC_SENSORS = "exoskeleton/bubble/sensors";

// ==================== 函数声明 ====================
void app_main();
void bubble_task(void *pvParameter);
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void publishSensorData();
void processCommand(cJSON* command);
void sprayBubbles(int duration, int intensity);
void sendStatus(const char* state, const char* message);
void initializeHardware();

// ==================== 主程序 ====================
void app_main() {
    DebugHelper::initialize();
    
    // 初始化传感器滤波器
    sensor_filter_init(&pressureFilter);
    sensor_filter_init(&flowFilter);
    sensor_filter_init(&tankLevelFilter);
    
    // 初始化硬件
    initializeHardware();
    
    // 初始化MQTT助手
    mqtt_helper_init(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT, 
                    "BubbleMachineClient", mqttCallback);
    
    // 连接WiFi和MQTT
    if (mqtt_helper_connect_wifi() && mqtt_helper_connect_broker()) {
        mqtt_helper_subscribe(TOPIC_COMMAND);
    }
    
    DebugHelper::info("泡泡机模块初始化完成");
    sendStatus("IDLE", "系统启动");
    
    // 创建主循环任务
    xTaskCreate(bubble_task, "bubble_task", 4096, NULL, 5, NULL);
}

void initializeHardware() {
    // 配置ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(FLOW_SENSOR_PIN, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(TANK_LEVEL_PIN, ADC_ATTEN_DB_0);
    
    // 配置压力传感器GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PRESSURE_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // 配置喷嘴PWM
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
        // 处理MQTT消息
        mqtt_helper_loop();
        
        static unsigned long lastUpdate = 0;
        unsigned long current_time = (unsigned long)(esp_timer_get_time() / 1000);
        if (current_time - lastUpdate > 1000) { // 每秒更新传感器数据
            publishSensorData();
            lastUpdate = current_time;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ==================== MQTT 回调 ====================
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    // 处理内部重新订阅消息
    if (strcmp(topic, "internal/resubscribe") == 0) {
        DebugHelper::info("重新订阅主题: %s", TOPIC_COMMAND);
        mqtt_helper_subscribe(TOPIC_COMMAND);
        return;
    }
    
    // 创建以null结尾的字符串
    char* json_string = (char*)malloc(length + 1);
    memcpy(json_string, payload, length);
    json_string[length] = '\0';
    
    // 解析 JSON
    cJSON* json = cJSON_Parse(json_string);
    free(json_string);
    
    if (json == NULL) {
        DebugHelper::error("JSON解析失败");
        return;
    }
    
    // 处理命令
    if (strcmp(topic, TOPIC_COMMAND) == 0) {
        processCommand(json);
    }
    
    cJSON_Delete(json);
}

// ==================== 传感器数据 ====================
void publishSensorData() {
    // 创建JSON对象
    cJSON* json = cJSON_CreateObject();
    
    // 读取原始传感器值
    float rawPressure = gpio_get_level(PRESSURE_PIN);
    float rawFlow = adc1_get_raw(FLOW_SENSOR_PIN);
    float rawTank = adc1_get_raw(TANK_LEVEL_PIN);
    
    // 滤波
    sensor_filter_add_value(&pressureFilter, rawPressure);
    sensor_filter_add_value(&flowFilter, rawFlow);
    sensor_filter_add_value(&tankLevelFilter, rawTank);
    
    // 校准
    float calibratedPressure = calibrate_pressure(sensor_filter_get_filtered(&pressureFilter));
    float calibratedFlow = calibrate_flow(sensor_filter_get_filtered(&flowFilter));
    float calibratedTank = calibrate_flow(sensor_filter_get_filtered(&tankLevelFilter));
    
    // 添加到JSON
    cJSON_AddNumberToObject(json, "flow_rate", calibratedFlow);
    cJSON_AddNumberToObject(json, "tank_level", calibratedTank);
    cJSON_AddNumberToObject(json, "system_pressure", calibratedPressure);
    
    // 序列化并发布
    char* json_string = cJSON_Print(json);
    mqtt_helper_publish(TOPIC_SENSORS, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// ==================== 命令处理 ====================
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

// ==================== 泡泡喷洒控制 ====================
void sprayBubbles(int duration, int intensity) {
    DebugHelper::info("喷洒修复液, 持续时间: %dms, 强度: %d%%", duration, intensity);
    sendStatus("SPRAYING", "喷洒修复液...");
    
    // 根据强度调整喷嘴 (PWM控制)
    int pwmValue = (intensity * 255) / 100;  // 映射0-100%到0-255
    ledc_set_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL, pwmValue);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL);
    
    // 记录开始时间
    unsigned long startTime = (unsigned long)(esp_timer_get_time() / 1000);
    
    // 喷洒过程监控
    while ((unsigned long)(esp_timer_get_time() / 1000) - startTime < duration) {
        // 检查压力是否正常
        int pressure = gpio_get_level(PRESSURE_PIN);
        if (pressure == 0) { // 压力过低
            DebugHelper::error("系统压力不足: %d", pressure);
            sendStatus("ERROR", "系统压力不足");
            break;
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    // 关闭喷嘴
    ledc_set_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, NOZZLE_PWM_CHANNEL);
    DebugHelper::info("喷洒完成");
    sendStatus("COMPLETED", "喷洒完成");
}

// ==================== 状态报告 ====================
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
    
    DebugHelper::info("状态报告: %s - %s", state, message);
}
