/**
 * ESP32 泡泡机模块控制器
 * 负责修复液喷洒控制
 */

#include "mqtt_helper.h"
#include "debug_helper.h"
#include "sensor_filter.h"
#include "sensor_calibration.h"
#include <ArduinoJson.h>

// 传感器过滤器实例
SensorFilter pressureFilter;
SensorFilter flowFilter;
SensorFilter tankLevelFilter;

// ==================== 硬件配置 ====================
#define NOZZLE_PIN         12    // 喷嘴控制引脚
#define FLOW_SENSOR_PIN    36    // 流量传感器 (ADC)
#define TANK_LEVEL_PIN     39    // 储液罐液位传感器 (ADC)
#define PRESSURE_PIN       14    // 系统压力传感器 (ADC)

// PWM配置
#define NOZZLE_PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

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
void setup();
void loop();
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void publishSensorData();
void processCommand(JsonDocument& command);
void sprayBubbles(int duration, int intensity);
void sendStatus(const char* state, const char* message);

// ==================== 主程序 ====================
void setup() {
  DebugHelper::initialize();
  
  // 初始化引脚
  pinMode(FLOW_SENSOR_PIN, INPUT);
  pinMode(TANK_LEVEL_PIN, INPUT);
  pinMode(PRESSURE_PIN, INPUT);
  
  // 配置喷嘴PWM
  ledcSetup(NOZZLE_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(NOZZLE_PIN, NOZZLE_PWM_CHANNEL);
  ledcWrite(NOZZLE_PWM_CHANNEL, 0);  // 初始关闭
  
  // 初始化MQTT助手
  mqtt_helper_init(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT, 
                  "BubbleMachineClient", mqttCallback);
  
  // 连接WiFi和MQTT
  if (mqtt_helper_connect_wifi() && mqtt_helper_connect_broker()) {
    mqtt_helper_subscribe(TOPIC_COMMAND);
  }
  
  DebugHelper::info("泡泡机模块初始化完成");
  sendStatus("IDLE", "系统启动");
}

void loop() {
  // 处理MQTT消息
  mqtt_helper_loop();
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) { // 每秒更新传感器数据
    publishSensorData();
    lastUpdate = millis();
  }
  
  delay(10);
}

// ==================== MQTT 回调 ====================
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  // 处理内部重新订阅消息
  if (strcmp(topic, "internal/resubscribe") == 0) {
    DebugHelper::info("重新订阅主题: %s", TOPIC_COMMAND);
    mqtt_helper_subscribe(TOPIC_COMMAND);
    return;
  }
  
  // 解析 JSON 指令
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload, length);
  
  // 处理命令
  if (strcmp(topic, TOPIC_COMMAND) == 0) {
    processCommand(doc);
  }
}

// ==================== 传感器数据 ====================
void publishSensorData() {
  // 创建JSON对象
  DynamicJsonDocument doc(256);
  
  // 读取原始传感器值
  float rawPressure = analogRead(PRESSURE_PIN);
  float rawFlow = analogRead(FLOW_SENSOR_PIN);
  float rawTank = analogRead(TANK_LEVEL_PIN);
  
  // 滤波
  pressureFilter.addValue(rawPressure);
  flowFilter.addValue(rawFlow);
  tankLevelFilter.addValue(rawTank);
  
  // 校准
  float calibratedPressure = calibrate_pressure(pressureFilter.getFiltered());
  float calibratedFlow = calibrate_flow(flowFilter.getFiltered());
  float calibratedTank = calibrate_flow(tankLevelFilter.getFiltered());
  
  // 添加到JSON
  doc["flow_rate"] = calibratedFlow;
  doc["tank_level"] = calibratedTank;
  doc["system_pressure"] = calibratedPressure;
  
  // 序列化并发布
  char buffer[256];
  serializeJson(doc, buffer);
  mqtt_helper_publish(TOPIC_SENSORS, buffer);
}

// ==================== 命令处理 ====================
void processCommand(JsonDocument& command) {
  const char* action = command["action"];
  
  if (strcmp(action, "spray") == 0) {
    int duration = command["params"]["duration"];
    int intensity = command["params"]["intensity"];
    sprayBubbles(duration, intensity);
  }
}

// ==================== 泡泡喷洒控制 ====================
void sprayBubbles(int duration, int intensity) {
  DebugHelper::info("喷洒修复液, 持续时间: %dms, 强度: %d%%", duration, intensity);
  sendStatus("SPRAYING", "喷洒修复液...");
  
  // 根据强度调整喷嘴 (PWM控制)
  int pwmValue = map(intensity, 0, 100, 50, 255);
  ledcWrite(NOZZLE_PWM_CHANNEL, pwmValue);
  
  // 记录开始时间
  unsigned long startTime = millis();
  
  // 喷洒过程监控
  while (millis() - startTime < duration) {
    // 检查压力是否正常
    int pressure = analogRead(PRESSURE_PIN);
    if (pressure < 100) { // 压力过低阈值
      DebugHelper::error("系统压力不足: %d", pressure);
      sendStatus("ERROR", "系统压力不足");
      break;
    }
    
    delay(100);
  }
  
  // 关闭喷嘴
  ledcWrite(NOZZLE_PWM_CHANNEL, 0);
  DebugHelper::info("喷洒完成");
  sendStatus("COMPLETED", "喷洒完成");
}

// ==================== 状态报告 ====================
void sendStatus(const char* state, const char* message) {
  DynamicJsonDocument doc(256);
  doc["module"] = "bubble";
  doc["state"] = state;
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  mqtt_helper_publish(TOPIC_STATUS, buffer);
  
  DebugHelper::info("状态报告: %s - %s", state, message);
}
