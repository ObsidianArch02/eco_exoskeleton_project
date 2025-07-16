/**
 * ESP32 注射模块控制器
 * 负责土壤注射操作
 */

#include "mqtt_helper.h"
#include <ArduinoJson.h>

// ==================== 硬件配置 ====================
#define MOTOR_PIN          12    // 注射电机控制 (PWM)
#define DEPTH_SENSOR_PIN   36    // 注射深度传感器 (ADC)
#define PRESSURE_PIN       39    // 注射压力传感器 (ADC)
#define NEEDLE_FEEDBACK_PIN 14   // 针头位置反馈

// PWM配置
#define MOTOR_PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// ==================== 网络配置 ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT 主题
const char* TOPIC_COMMAND = "exoskeleton/injection/command";
const char* TOPIC_STATUS = "exoskeleton/injection/status";
const char* TOPIC_SENSORS = "exoskeleton/injection/sensors";

// ==================== 函数声明 ====================
void setup();
void loop();
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void publishSensorData();
void processCommand(JsonDocument& command);
void injectSoil(int depth, int pressure);
void sendStatus(const char* state, const char* message);

// ==================== 主程序 ====================
void setup() {
  Serial.begin(115200);
  
  // 初始化引脚
  pinMode(DEPTH_SENSOR_PIN, INPUT);
  pinMode(PRESSURE_PIN, INPUT);
  pinMode(NEEDLE_FEEDBACK_PIN, INPUT_PULLUP);
  
  // 配置电机PWM
  ledcSetup(MOTOR_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PIN, MOTOR_PWM_CHANNEL);
  ledcWrite(MOTOR_PWM_CHANNEL, 0);  // 初始关闭
  
  // 初始化MQTT助手
  mqtt_helper_init(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT, 
                  "InjectionClient", mqttCallback);
  
  // 连接WiFi和MQTT
  if (mqtt_helper_connect_wifi() && mqtt_helper_connect_broker()) {
    mqtt_helper_subscribe(TOPIC_COMMAND);
  }
  
  Serial.println("注射模块初始化完成");
  sendStatus("IDLE", "系统启动");
}

void loop() {
  // 处理MQTT消息
  mqtt_helper_loop();
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 200) { // 5Hz 更新传感器数据
    publishSensorData();
    lastUpdate = millis();
  }
  
  delay(10);
}

// ==================== 网络连接 ====================
// 与温室模块相同，省略重复代码...

// ==================== MQTT 回调 ====================
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
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
  
  // 读取传感器
  doc["depth"] = analogRead(DEPTH_SENSOR_PIN);
  doc["pressure"] = analogRead(PRESSURE_PIN);
  doc["needle_position"] = digitalRead(NEEDLE_FEEDBACK_PIN);
  
  // 序列化并发布
  char buffer[256];
  serializeJson(doc, buffer);
  mqtt_helper_publish(TOPIC_SENSORS, buffer);
}

// ==================== 命令处理 ====================
void processCommand(JsonDocument& command) {
  const char* action = command["action"];
  
  if (strcmp(action, "inject") == 0) {
    int depth = command["params"]["depth"];
    int pressure = command["params"]["pressure"];
    injectSoil(depth, pressure);
  }
  else if (strcmp(action, "retract") == 0) {
    // 收回针头逻辑
  }
}

// ==================== 注射控制 ====================
void injectSoil(int targetDepth, int targetPressure) {
  sendStatus("INJECTING", "开始注射...");
  
  // 激活电机 (PWM控制)
  int motorPower = map(targetPressure, 0, 300, 150, 255);
  ledcWrite(MOTOR_PWM_CHANNEL, motorPower);
  
  // 监控注射深度
  unsigned long startTime = millis();
  while (true) {
    int currentDepth = analogRead(DEPTH_SENSOR_PIN);
    
    // 达到目标深度
    if (currentDepth >= targetDepth) {
      break;
    }
    
    // 超时检查
    if (millis() - startTime > 10000) { // 10秒超时
      sendStatus("ERROR", "注射超时");
      break;
    }
    
    delay(50);
  }
  
  // 停止电机
  ledcWrite(MOTOR_PWM_CHANNEL, 0);
  sendStatus("COMPLETED", "注射完成");
}

// ==================== 状态报告 ====================
void sendStatus(const char* state, const char* message) {
  DynamicJsonDocument doc(256);
  doc["module"] = "injection";
  doc["state"] = state;
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  mqtt_helper_publish(TOPIC_STATUS, buffer);
  
  Serial.printf("状态报告: %s - %s\n", state, message);
}
