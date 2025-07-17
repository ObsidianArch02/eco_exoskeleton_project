"""
传感器数据收集器模块

该模块专门用于收集、管理和提供传感器数据。
提供MQTT数据订阅、数据缓存、历史数据管理等功能，
为数据处理算法提供统一的数据接口。
"""

import json
import time
import logging
import threading
from collections import deque, defaultdict
from typing import Dict, List, Optional, Callable, Any
import paho.mqtt.client as mqtt
from config import (
    MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASS,
    TOPIC_GREENHOUSE_SENSORS, TOPIC_INJECTION_SENSORS, TOPIC_BUBBLE_SENSORS
)

logger = logging.getLogger(__name__)

class SensorDataBuffer:
    """传感器数据缓冲区"""
    
    def __init__(self, max_size: int = 1000):
        self.max_size = max_size
        self.data_buffer = deque(maxlen=max_size)
        self.module_buffers = defaultdict(lambda: deque(maxlen=max_size))
        self.lock = threading.Lock()
    
    def add_sensor_data(self, module: str, data: dict, timestamp: Optional[float] = None):
        """添加传感器数据"""
        if timestamp is None:
            timestamp = time.time()
        
        data_entry = {
            'module': module,
            'timestamp': timestamp,
            'data': data
        }
        
        with self.lock:
            self.data_buffer.append(data_entry)
            self.module_buffers[module].append(data_entry)
    
    def get_latest_data(self, module: Optional[str] = None) -> Optional[dict]:
        """获取最新数据"""
        with self.lock:
            if module:
                if self.module_buffers[module]:
                    return self.module_buffers[module][-1]
            else:
                if self.data_buffer:
                    return self.data_buffer[-1]
        return None
    
    def get_historical_data(self, module: Optional[str] = None, count: int = 100) -> List[dict]:
        """获取历史数据"""
        with self.lock:
            if module:
                return list(self.module_buffers[module])[-count:]
            else:
                return list(self.data_buffer)[-count:]
    
    def get_data_in_timerange(self, start_time: float, end_time: float, module: Optional[str] = None) -> List[dict]:
        """获取时间范围内的数据"""
        with self.lock:
            source = self.module_buffers[module] if module else self.data_buffer
            return [entry for entry in source 
                   if start_time <= entry['timestamp'] <= end_time]

class SensorCollector:
    """传感器数据收集器"""
    
    def __init__(self, buffer_size: int = 1000):
        self.buffer = SensorDataBuffer(buffer_size)
        self.client = mqtt.Client(client_id="sensor_collector")
        self.connected = False
        self.data_callbacks: List[Callable[[str, dict], None]] = []
        self.running = False
        
        # MQTT事件回调
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        
        # 设置认证
        self.client.username_pw_set(MQTT_USER, MQTT_PASS)
        
        logger.info("传感器数据收集器初始化完成")
    
    def add_data_callback(self, callback: Callable[[str, dict], None]):
        """添加数据回调函数"""
        self.data_callbacks.append(callback)
    
    def remove_data_callback(self, callback: Callable[[str, dict], None]):
        """移除数据回调函数"""
        if callback in self.data_callbacks:
            self.data_callbacks.remove(callback)
    
    def connect(self) -> bool:
        """连接到MQTT服务器"""
        try:
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_start()
            self.running = True
            
            # 等待连接完成
            timeout = 5.0
            start_time = time.time()
            while not self.connected and (time.time() - start_time) < timeout:
                time.sleep(0.1)
            
            if self.connected:
                logger.info("传感器收集器MQTT连接成功")
                return True
            else:
                logger.error("传感器收集器MQTT连接超时")
                return False
                
        except Exception as e:
            logger.error(f"传感器收集器MQTT连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开MQTT连接"""
        self.running = False
        if self.connected:
            self.client.loop_stop()
            self.client.disconnect()
            logger.info("传感器收集器已断开连接")
    
    def _on_connect(self, client, userdata, flags, rc):
        """MQTT连接回调"""
        if rc == 0:
            self.connected = True
            # 订阅所有传感器主题
            sensor_topics = [
                TOPIC_GREENHOUSE_SENSORS,
                TOPIC_INJECTION_SENSORS,
                TOPIC_BUBBLE_SENSORS
            ]
            
            for topic in sensor_topics:
                client.subscribe(topic)
                logger.info(f"已订阅传感器主题: {topic}")
        else:
            logger.error(f"MQTT连接失败，错误码: {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """MQTT断开连接回调"""
        self.connected = False
        logger.warning("传感器收集器MQTT连接断开")
    
    def _on_message(self, client, userdata, msg):
        """MQTT消息回调"""
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode('utf-8'))
            timestamp = time.time()
            
            # 确定模块名称
            module = self._extract_module_name(topic)
            
            # 存储到缓冲区
            self.buffer.add_sensor_data(module, payload, timestamp)
            
            # 调用所有注册的回调函数
            for callback in self.data_callbacks:
                try:
                    callback(module, payload)
                except Exception as e:
                    logger.error(f"数据回调函数执行失败: {e}")
            
            logger.debug(f"收到 {module} 模块传感器数据: {payload}")
            
        except Exception as e:
            logger.error(f"处理传感器数据失败: {e}")
    
    def _extract_module_name(self, topic: str) -> str:
        """从MQTT主题提取模块名称"""
        if "greenhouse" in topic:
            return "greenhouse"
        elif "injection" in topic:
            return "injection"
        elif "bubble" in topic:
            return "bubble"
        else:
            return "unknown"
    
    def get_latest_data(self, module: Optional[str] = None) -> Optional[dict]:
        """获取最新传感器数据"""
        return self.buffer.get_latest_data(module)
    
    def get_historical_data(self, module: Optional[str] = None, count: int = 100) -> List[dict]:
        """获取历史传感器数据"""
        return self.buffer.get_historical_data(module, count)
    
    def get_data_in_timerange(self, start_time: float, end_time: float, module: Optional[str] = None) -> List[dict]:
        """获取时间范围内的传感器数据"""
        return self.buffer.get_data_in_timerange(start_time, end_time, module)
    
    def get_buffer_status(self) -> Dict[str, Any]:
        """获取缓冲区状态信息"""
        with self.buffer.lock:
            return {
                'total_entries': len(self.buffer.data_buffer),
                'module_entries': {module: len(buf) for module, buf in self.buffer.module_buffers.items()},
                'buffer_size': self.buffer.max_size,
                'connected': self.connected,
                'running': self.running
            }

# 全局传感器收集器实例
_sensor_collector = None

def get_sensor_collector() -> SensorCollector:
    """获取全局传感器收集器实例"""
    global _sensor_collector
    if _sensor_collector is None:
        _sensor_collector = SensorCollector()
    return _sensor_collector 