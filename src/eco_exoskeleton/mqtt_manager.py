import json
import logging
import paho.mqtt.client as mqtt
from eco_exoskeleton.models import SensorData, ModuleStatus, Command, ModuleState
from eco_exoskeleton.config import *

logger = logging.getLogger(__name__)

class MQTTManager:
    def __init__(self, decision_system):
        self.decision_system = decision_system
        self.client = mqtt.Client()
        self.sensor_data = SensorData()
        self.connected = False
        
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.username_pw_set(MQTT_USER, MQTT_PASS)
    
    def connect(self):
        try:
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_start()
            self.connected = True
            return True
        except Exception as e:
            logger.error("MQTT 连接失败", exc_info=e)
            return False
    
    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            client.subscribe([
                (TOPIC_GREENHOUSE_SENSORS, 0),
                (TOPIC_GREENHOUSE_STATUS, 0),
                (TOPIC_INJECTION_SENSORS, 0),
                (TOPIC_INJECTION_STATUS, 0),
                (TOPIC_BUBBLE_SENSORS, 0),
                (TOPIC_BUBBLE_STATUS, 0)
            ])
            self.connected = True
        else:
            logger.error(f"连接失败，错误码: {rc}")
    
    def _on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode("utf-8")
            data = json.loads(payload)
            
            if msg.topic == TOPIC_GREENHOUSE_SENSORS:
                self._process_greenhouse_sensors(data)
            elif msg.topic == TOPIC_GREENHOUSE_STATUS:
                self._process_greenhouse_status(data)
            elif msg.topic == TOPIC_INJECTION_SENSORS:
                self._process_injection_sensors(data)
            elif msg.topic == TOPIC_INJECTION_STATUS:
                self._process_injection_status(data)
            elif msg.topic == TOPIC_BUBBLE_SENSORS:
                self._process_bubble_sensors(data)
            elif msg.topic == TOPIC_BUBBLE_STATUS:
                self._process_bubble_status(data)
                
        except Exception as e:
            logger.error("消息处理错误", exc_info=e)
    
    def _process_greenhouse_sensors(self, data: dict):
        self.sensor_data.temperature = data.get("temperature", 25.0)
        self.sensor_data.humidity = data.get("humidity", 50.0)
        self.decision_system.update_sensor_data(self.sensor_data)
    
    def _process_greenhouse_status(self, data: dict):
        state_value = data["state"]
        if not isinstance(state_value, ModuleState):
            state_value = ModuleState(state_value)
        status = ModuleStatus(
            module="greenhouse",
            state=state_value,
            message=data["message"],
            timestamp=data["timestamp"]
        )
        self.decision_system.update_module_status(status)
    
    def _process_injection_sensors(self, data: dict):
        self.sensor_data.soil_moisture = data.get("soil_moisture", 0.0)
        self.sensor_data.injection_depth = data.get("current_depth", 0.0)
        self.decision_system.update_sensor_data(self.sensor_data)
    
    def _process_injection_status(self, data: dict):
        state_value = data["state"]
        if not isinstance(state_value, ModuleState):
            state_value = ModuleState(state_value)
        status = ModuleStatus(
            module="injection",
            state=state_value,
            message=data["message"],
            timestamp=data["timestamp"],
            data={"depth": data.get("depth", 0), "pressure": data.get("pressure", 0)}
        )
        self.decision_system.update_module_status(status)
    
    def _process_bubble_sensors(self, data: dict):
        self.sensor_data.bubble_flow = data.get("flow_rate", 0.0)
        self.decision_system.update_sensor_data(self.sensor_data)
    
    def _process_bubble_status(self, data: dict):
        state_value = data["state"]
        if not isinstance(state_value, ModuleState):
            state_value = ModuleState(state_value)
        status = ModuleStatus(
            module="bubble",
            state=state_value,
            message=data["message"],
            timestamp=data["timestamp"],
            data={"duration": data.get("duration", 0), "intensity": data.get("intensity", 0)}
        )
        self.decision_system.update_module_status(status)
    
    def send_command(self, command: Command) -> bool:
        if not self.connected:
            return False
            
        topic = ""
        if command.module == "greenhouse":
            topic = TOPIC_GREENHOUSE_COMMAND
        elif command.module == "injection":
            topic = TOPIC_INJECTION_COMMAND
        elif command.module == "bubble":
            topic = TOPIC_BUBBLE_COMMAND
        else:
            return False
            
        payload = json.dumps({
            "action": command.action,
            "params": command.params
        })
        
        self.client.publish(topic, payload)
        return True

    def disconnect(self):
        if self.connected:
            self.client.loop_stop()
            self.client.disconnect()
