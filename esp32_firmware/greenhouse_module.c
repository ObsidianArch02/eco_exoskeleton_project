/**
 * ESP32 温室模块控制器
 * 负责可发射折叠温室的部署与收回
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==================== 硬件配置 ====================
#define DEPLOY_PIN         12    // 温室展开控制引脚
#define RETRACT_PIN        13    // 温室收回控制引脚
#define DEPLOY_FEEDBACK_PIN 14   // 展开到位反馈
#define RETRACT_FEEDBACK_PIN 15  // 收回到位反馈
#define TEMP_SENSOR_PIN    36    // 温度传感器 (ADC)
#define HUMIDITY_PIN       39    // 湿度传感器 (ADC)

// ==================== 网络配置 ====================
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
const char* MQTT_BROKER = "192.168.1.100";
const int   MQTT_PORT = 1883;

// MQTT 主题
const char* TOPIC_COMMAND = "exoskeleton/greenhouse/command";
const char* TOPIC_STATUS = "exoskeleton/greenhouse/status";
const char* TOPIC_SENSORS = "exoskeleton/greenhouse/sensors";

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
void deployGreenhouse();
void retractGreenhouse();
void sendStatus(const char* state, const char* message);

// ==================== 主程序 ====================
void setup() {
  Serial.begin(115200);
  
  // 初始化引脚
  pinMode(DEPLOY_PIN, OUTPUT);
  pinMode(RETRACT_PIN, OUTPUT);
  pinMode(DEPLOY_FEEDBACK_PIN, INPUT_PULLUP);
  pinMode(RETRACT_FEEDBACK_PIN, INPUT_PULLUP);
  
  // 初始状态
  digitalWrite(DEPLOY_PIN, LOW);
  digitalWrite(RETRACT_PIN, LOW);
  
  // 连接网络
  connectToWiFi();
  
  // 设置 MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  Serial.println("温室模块初始化完成");
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
void connectToWiFi() {
  Serial.printf("连接WiFi: %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi连接成功!");
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("连接MQTT代理...");
    
    if (mqttClient.connect("ESP32_Greenhouse")) {
      Serial.println("MQTT连接成功!");
      mqttClient.subscribe(TOPIC_COMMAND);
    } else {
      Serial.print("失败, 错误码: ");
      Serial.print(mqttClient.state());
      Serial.println(" 5秒后重试...");
      delay(5000);
    }
  }
}

// ==================== MQTT 回调 ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("收到消息 [%s]: ", topic);
  
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
  doc["temperature"] = analogRead(TEMP_SENSOR_PIN) / 4095.0 * 100.0;
  doc["humidity"] = analogRead(HUMIDITY_PIN) / 4095.0 * 100.0;
  doc["deployed"] = digitalRead(DEPLOY_FEEDBACK_PIN) == HIGH;
  doc["retracted"] = digitalRead(RETRACT_FEEDBACK_PIN) == HIGH;
  
  // 序列化并发布
  char buffer[256];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_SENSORS, buffer);
  
  Serial.println("温室传感器数据已发布");
}

// ==================== 命令处理 ====================
void processCommand(JsonDocument& command) {
  const char* action = command["action"];
  
  Serial.printf("执行命令: %s\n", action);
  
  if (strcmp(action, "deploy") == 0) {
    deployGreenhouse();
  }
  else if (strcmp(action, "retract") == 0) {
    retractGreenhouse();
  }
}

// ==================== 温室控制 ====================
void deployGreenhouse() {
  sendStatus("DEPLOYING", "展开温室...");
  
  // 激活展开机构
  digitalWrite(DEPLOY_PIN, HIGH);
  
  // 等待展开完成信号
  unsigned long startTime = millis();
  while (digitalRead(DEPLOY_FEEDBACK_PIN) == LOW) {
    if (millis() - startTime > 5000) { // 5秒超时
      sendStatus("ERROR", "温室展开超时");
      digitalWrite(DEPLOY_PIN, LOW);
      return;
    }
    delay(100);
  }
  
  digitalWrite(DEPLOY_PIN, LOW);
  sendStatus("DEPLOYED", "温室部署完成");
}

void retractGreenhouse() {
  sendStatus("RETRACTING", "收回温室...");
  
  // 激活收回机构
  digitalWrite(RETRACT_PIN, HIGH);
  
  // 等待收回完成信号
  unsigned long startTime = millis();
  while (digitalRead(RETRACT_FEEDBACK_PIN) == LOW) {
    if (millis() - startTime > 5000) { // 5秒超时
      sendStatus("ERROR", "温室收回超时");
      digitalWrite(RETRACT_PIN, LOW);
      return;
    }
    delay(100);
  }
  
  digitalWrite(RETRACT_PIN, LOW);
  sendStatus("RETRACTED", "温室收回完成");
}

// ==================== 状态报告 ====================
void sendStatus(const char* state, const char* message) {
  DynamicJsonDocument doc(256);
  doc["module"] = "greenhouse";
  doc["state"] = state;
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_STATUS, buffer);
  
  Serial.printf("状态报告: %s - %s\n", state, message);
}
