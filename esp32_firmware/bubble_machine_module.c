/**
 * ESP32 泡泡机模块控制器
 * 负责修复液喷洒控制
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==================== 硬件配置 ====================
#define NOZZLE_PIN         12    // 喷嘴控制引脚
#define FLOW_SENSOR_PIN    36    // 流量传感器 (ADC)
#define TANK_LEVEL_PIN     39    // 储液罐液位传感器 (ADC)
#define PRESSURE_PIN       14    // 系统压力传感器 (ADC)

// ==================== 网络配置 ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT 主题
const char* TOPIC_COMMAND = "exoskeleton/bubble/command";
const char* TOPIC_STATUS = "exoskeleton/bubble/status";
const char* TOPIC_SENSORS = "exoskeleton/bubble/sensors";

// ==================== 全局对象 ====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ==================== 函数声明 ====================
void setup();
void loop();
void connectToWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void publishSensorData();
void processCommand(JsonDocument& command);
void sprayBubbles(int duration, int intensity);
void sendStatus(const char* state, const char* message);

// ==================== 主程序 ====================
void setup() {
  Serial.begin(115200);
  
  // 初始化引脚
  pinMode(NOZZLE_PIN, OUTPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT);
  pinMode(TANK_LEVEL_PIN, INPUT);
  pinMode(PRESSURE_PIN, INPUT);
  
  // 初始状态
  digitalWrite(NOZZLE_PIN, LOW);
  
  // 连接网络
  connectToWiFi();
  
  // 设置 MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  Serial.println("泡泡机模块初始化完成");
  sendStatus("IDLE", "系统启动");
}

void loop() {
  // 维护 MQTT 连接
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) { // 每秒更新传感器数据
    publishSensorData();
    lastUpdate = millis();
  }
  
  delay(10);
}

// ==================== 网络连接 ====================
// 与温室模块相同，省略重复代码...

// ==================== MQTT 回调 ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
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
  doc["flow_rate"] = analogRead(FLOW_SENSOR_PIN);
  doc["tank_level"] = analogRead(TANK_LEVEL_PIN);
  doc["system_pressure"] = analogRead(PRESSURE_PIN);
  
  // 序列化并发布
  char buffer[256];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_SENSORS, buffer);
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
  sendStatus("SPRAYING", "喷洒修复液...");
  
  // 根据强度调整喷嘴 (PWM控制)
  analogWrite(NOZZLE_PIN, map(intensity, 0, 100, 50, 255));
  
  // 记录开始时间
  unsigned long startTime = millis();
  
  // 喷洒过程监控
  while (millis() - startTime < duration) {
    // 检查压力是否正常
    int pressure = analogRead(PRESSURE_PIN);
    if (pressure < 100) { // 压力过低阈值
      sendStatus("ERROR", "系统压力不足");
      break;
    }
    
    delay(100);
  }
  
  // 关闭喷嘴
  analogWrite(NOZZLE_PIN, 0);
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
  mqttClient.publish(TOPIC_STATUS, buffer);
  
  Serial.printf("状态报告: %s - %s\n", state, message);
}
