import os

# MQTT configuration
MQTT_BROKER = "192.168.1.100"
MQTT_PORT = 1883
MQTT_USER = os.getenv("MQTT_USER", "admin")
MQTT_PASS = os.getenv("MQTT_PASS", "password")

# Greenhouse module topics
TOPIC_GREENHOUSE_SENSORS = "exoskeleton/greenhouse/sensors"
TOPIC_GREENHOUSE_STATUS = "exoskeleton/greenhouse/status"
TOPIC_GREENHOUSE_COMMAND = "exoskeleton/greenhouse/command"

# Injection module topics
TOPIC_INJECTION_SENSORS = "exoskeleton/injection/sensors"
TOPIC_INJECTION_STATUS = "exoskeleton/injection/status"
TOPIC_INJECTION_COMMAND = "exoskeleton/injection/command"

# Bubble machine module topics
TOPIC_BUBBLE_SENSORS = "exoskeleton/bubble/sensors"
TOPIC_BUBBLE_STATUS = "exoskeleton/bubble/status"
TOPIC_BUBBLE_COMMAND = "exoskeleton/bubble/command"

# System parameters
CONTROL_LOOP_FREQ = 10  # Hz
DECISION_INTERVAL = 1.0  # seconds
