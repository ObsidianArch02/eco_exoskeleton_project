import threading
import time
import logging
from .decision_system import CentralDecisionSystem
from .mqtt_manager import MQTTManager
from ..models import Command, SensorData
from ..config import CONTROL_LOOP_FREQ

"""Ecological Exoskeleton System Controller
This module manages the overall system, including decision making,
sensor data processing, and command execution.
It handles MQTT communication and can operate in test mode with simulated sensor data.
"""

logger = logging.getLogger(__name__)

class EcologicalExoskeletonSystem:
    def __init__(self, test_mode=False):
        self.decision_system = CentralDecisionSystem()
        self.mqtt_manager = MQTTManager(self.decision_system)
        self.running = False
        self.control_thread = None
        self.test_mode = test_mode
        self.test_sensor_gen = None
        
    def start(self):
        if self.running:
            return False
            
        if not self.mqtt_manager.connect():
            return False
            
        self.running = True
        self.control_thread = threading.Thread(target=self._control_loop)
        self.control_thread.daemon = True
        self.control_thread.start()
        # if self.test_mode:
        #     self.test_sensor_gen = TestSensorGenerator(self.decision_system)
        #     self.test_sensor_gen.daemon = True
        #     self.test_sensor_gen.start()
        return True
    
    def stop(self):
        if not self.running:
            return
            
        self.running = False
        if self.control_thread and self.control_thread.is_alive():
            self.control_thread.join(timeout=5.0)
        # if self.test_sensor_gen:
        #     self.test_sensor_gen.stop()
        #     self.test_sensor_gen = None
        self.mqtt_manager.disconnect()
    
    def _control_loop(self):
        while self.running:
            try:
                command = self.decision_system.make_decision()
                if command:
                    self.mqtt_manager.send_command(command)
                time.sleep(1.0 / CONTROL_LOOP_FREQ)
            except Exception as e:
                logger.exception("控制循环错误")
                self.emergency_stop()
                time.sleep(1)
    
    def emergency_stop(self):
        commands = [
            Command("greenhouse", "emergency_stop", {}),
            Command("injection", "emergency_stop", {}),
            Command("bubble", "emergency_stop", {})
        ]
        
        for cmd in commands:
            self.mqtt_manager.send_command(cmd)
        
        self.stop()
