import threading
import time
import logging
from decision_system import CentralDecisionSystem
from mqtt_manager import MQTTManager
from sensor_collector import get_sensor_collector
from algorithm_manager import get_algorithm_manager
from models import Command
from config import CONTROL_LOOP_FREQ

"""Ecological Exoskeleton System Controller
This module manages the overall system, including decision making,
sensor data processing, and command execution.
It handles MQTT communication and can operate in test mode with simulated sensor data.

Updated to include:
- Sensor data collection and buffering
- Data processing algorithms
- Algorithm management and pipeline processing
"""

logger = logging.getLogger(__name__)

class EcologicalExoskeletonSystem:
    def __init__(self, test_mode=False):
        self.decision_system = CentralDecisionSystem()
        self.mqtt_manager = MQTTManager(self.decision_system)
        self.sensor_collector = get_sensor_collector()
        self.algorithm_manager = get_algorithm_manager()
        
        self.running = False
        self.control_thread = None
        self.test_mode = test_mode
        self.test_sensor_gen = None
        
        logger.info("生态外骨骼系统初始化完成 (包含数据处理模块)")
        
    def start(self):
        if self.running:
            return False
            
        logger.info("启动生态外骨骼系统...")
        
        # 启动MQTT管理器
        if not self.mqtt_manager.connect():
            logger.error("MQTT管理器启动失败")
            return False
        
        # 启动传感器收集器
        if not self.sensor_collector.connect():
            logger.error("传感器收集器启动失败")
            return False
        
        # 启动算法管理器
        if not self.algorithm_manager.start():
            logger.error("算法管理器启动失败")
            return False
        
        # 创建默认数据处理管道
        self._setup_default_pipelines()
        
        self.running = True
        self.control_thread = threading.Thread(target=self._control_loop)
        self.control_thread.daemon = True
        self.control_thread.start()
        
        logger.info("生态外骨骼系统启动成功")
        return True
    
    def _setup_default_pipelines(self):
        """设置默认的数据处理管道"""
        from algorithm_manager import ProcessingPipeline
        
        # 创建温度数据处理管道
        temp_pipeline = ProcessingPipeline(
            name="温度数据处理",
            algorithms=["温度滤波", "异常检测", "趋势分析"],
            input_modules=["greenhouse"]
        )
        
        # 创建湿度数据处理管道
        humidity_pipeline = ProcessingPipeline(
            name="湿度数据处理", 
            algorithms=["湿度滤波", "统计分析"],
            input_modules=["greenhouse"]
        )
        
        # 创建压力数据处理管道
        pressure_pipeline = ProcessingPipeline(
            name="压力数据处理",
            algorithms=["自适应滤波", "异常检测"],
            input_modules=["injection", "bubble"]
        )
        
        # 创建综合数据处理管道
        comprehensive_pipeline = ProcessingPipeline(
            name="综合数据处理",
            algorithms=["异常检测", "统计分析", "趋势分析"],
            input_modules=["greenhouse", "injection", "bubble"]
        )
        
        # 注册管道
        pipelines = [temp_pipeline, humidity_pipeline, pressure_pipeline, comprehensive_pipeline]
        for pipeline in pipelines:
            self.algorithm_manager.create_pipeline(pipeline)
            logger.info(f"创建数据处理管道: {pipeline.name}")
    
    def stop(self):
        if not self.running:
            return
            
        logger.info("停止生态外骨骼系统...")
        self.running = False
        
        # 停止控制循环
        if self.control_thread and self.control_thread.is_alive():
            self.control_thread.join(timeout=5.0)
        
        # 停止算法管理器
        self.algorithm_manager.stop()
        
        # 停止传感器收集器
        self.sensor_collector.disconnect()
        
        # 停止MQTT管理器
        self.mqtt_manager.disconnect()
        
        logger.info("生态外骨骼系统已停止")
    
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
    
    def get_system_status(self):
        """获取系统状态，包括数据处理模块状态"""
        status = {
            'system_running': self.running,
            'mqtt_connected': self.mqtt_manager.connected,
            'sensor_collector': self.sensor_collector.get_buffer_status(),
            'algorithm_manager': self.algorithm_manager.get_algorithm_status(),
            'decision_system': {
                'module_states': {name: status.state.value for name, status in self.decision_system.module_states.items()},
                'repair_plan_length': len(self.decision_system.repair_plan)
            }
        }
        return status
    
    def get_processed_data_summary(self):
        """获取处理后的数据摘要"""
        summary = {}
        
        # 获取最新的传感器数据
        for module in ['greenhouse', 'injection', 'bubble']:
            latest_data = self.sensor_collector.get_latest_data(module)
            if latest_data:
                summary[module] = {
                    'raw_data': latest_data['data'],
                    'timestamp': latest_data['timestamp']
                }
        
        # 获取算法处理结果
        algorithm_results = {}
        for algo_name in self.algorithm_manager.algorithm_configs.keys():
            results = self.algorithm_manager.get_algorithm_results(algo_name, count=1)
            if results:
                algorithm_results[algo_name] = {
                    'processed_value': results[-1].processed_value,
                    'confidence': results[-1].confidence,
                    'algorithm': results[-1].metadata.get('algorithm', 'unknown')
                }
        
        summary['algorithm_results'] = algorithm_results
        return summary
