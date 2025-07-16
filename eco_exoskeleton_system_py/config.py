import os

# MQTT 配置
MQTT_BROKER = "192.168.1.100"
MQTT_PORT = 1883
MQTT_USER = os.getenv("MQTT_USER", "admin")
MQTT_PASS = os.getenv("MQTT_PASS", "password")

# 温室模块主题
TOPIC_GREENHOUSE_SENSORS = "exoskeleton/greenhouse/sensors"
TOPIC_GREENHOUSE_STATUS = "exoskeleton/greenhouse/status"
TOPIC_GREENHOUSE_COMMAND = "exoskeleton/greenhouse/command"

# 注射模块主题
TOPIC_INJECTION_SENSORS = "exoskeleton/injection/sensors"
TOPIC_INJECTION_STATUS = "exoskeleton/injection/status"
TOPIC_INJECTION_COMMAND = "exoskeleton/injection/command"

# 泡泡机模块主题
TOPIC_BUBBLE_SENSORS = "exoskeleton/bubble/sensors"
TOPIC_BUBBLE_STATUS = "exoskeleton/bubble/status"
TOPIC_BUBBLE_COMMAND = "exoskeleton/bubble/command"

# 系统参数
CONTROL_LOOP_FREQ = 10  # Hz
DECISION_INTERVAL = 1.0  # 秒
