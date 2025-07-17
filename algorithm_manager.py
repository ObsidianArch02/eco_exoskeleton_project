"""
算法管理器模块

提供统一的算法接口，支持算法注册、数据处理管道、配置管理等功能。
允许用户动态添加、配置和组合不同的数据处理算法。
"""

import json
import time
import logging
from typing import Dict, List, Optional, Any, Callable, Type
from collections import defaultdict
from dataclasses import dataclass, asdict
from sensor_collector import get_sensor_collector, SensorCollector
from database_manager import get_database_manager
from data_processing import (
    ProcessingResult, MovingAverageFilter, KalmanFilter, 
    OutlierDetector, TrendAnalyzer, StatisticalAnalyzer, 
    DataFusionProcessor, AdaptiveFilter
)

logger = logging.getLogger(__name__)

@dataclass
class AlgorithmConfig:
    """算法配置"""
    name: str
    algorithm_type: str
    parameters: Dict[str, Any]
    enabled: bool = True
    priority: int = 0

@dataclass
class ProcessingPipeline:
    """数据处理管道"""
    name: str
    algorithms: List[str]
    input_modules: List[str]
    enabled: bool = True

class AlgorithmManager:
    """算法管理器"""
    
    def __init__(self, enable_database: bool = True):
        self.algorithms: Dict[str, Any] = {}
        self.algorithm_configs: Dict[str, AlgorithmConfig] = {}
        self.pipelines: Dict[str, ProcessingPipeline] = {}
        self.results_cache: Dict[str, List[ProcessingResult]] = defaultdict(list)
        self.sensor_collector: Optional[SensorCollector] = None
        self.running: bool = False
        self.enable_database: bool = enable_database
        
        # 初始化数据库管理器
        if self.enable_database:
            self.db_manager = get_database_manager()
        else:
            self.db_manager = None
        
        # 注册内置算法
        self._register_builtin_algorithms()
        
        logger.info(f"算法管理器初始化完成 (数据库: {'启用' if enable_database else '禁用'})")
    
    def _register_builtin_algorithms(self):
        """注册内置算法"""
        # 注册算法类型映射
        self.algorithm_types = {
            'moving_average': MovingAverageFilter,
            'kalman_filter': KalmanFilter,
            'outlier_detector': OutlierDetector,
            'trend_analyzer': TrendAnalyzer,
            'statistical_analyzer': StatisticalAnalyzer,
            'data_fusion': DataFusionProcessor,
            'adaptive_filter': AdaptiveFilter
        }
        
        # 创建默认算法配置
        default_configs = [
            AlgorithmConfig("温度滤波", "moving_average", {"window_size": 5}),
            AlgorithmConfig("湿度滤波", "kalman_filter", {"process_variance": 1e-5, "measurement_variance": 1e-1}),
            AlgorithmConfig("异常检测", "outlier_detector", {"window_size": 20, "threshold_multiplier": 2.0}),
            AlgorithmConfig("趋势分析", "trend_analyzer", {"window_size": 10}),
            AlgorithmConfig("统计分析", "statistical_analyzer", {"window_size": 50}),
            AlgorithmConfig("自适应滤波", "adaptive_filter", {"initial_alpha": 0.1})
        ]
        
        for config in default_configs:
            self.add_algorithm_config(config)
    
    def add_algorithm_config(self, config: AlgorithmConfig) -> bool:
        """添加算法配置"""
        try:
            if config.algorithm_type not in self.algorithm_types:
                logger.error(f"未知算法类型: {config.algorithm_type}")
                return False
            
            # 创建算法实例
            algorithm_class = self.algorithm_types[config.algorithm_type]
            algorithm_instance = algorithm_class(**config.parameters)
            
            self.algorithms[config.name] = algorithm_instance
            self.algorithm_configs[config.name] = config
            
            logger.info(f"添加算法配置: {config.name}")
            return True
            
        except Exception as e:
            logger.error(f"添加算法配置失败: {e}")
            return False
    
    def remove_algorithm_config(self, name: str) -> bool:
        """移除算法配置"""
        if name in self.algorithms:
            del self.algorithms[name]
            del self.algorithm_configs[name]
            logger.info(f"移除算法配置: {name}")
            return True
        return False
    
    def create_pipeline(self, pipeline: ProcessingPipeline) -> bool:
        """创建数据处理管道"""
        # 验证管道中的算法是否存在
        for algo_name in pipeline.algorithms:
            if algo_name not in self.algorithms:
                logger.error(f"管道 {pipeline.name} 中的算法 {algo_name} 不存在")
                return False
        
        self.pipelines[pipeline.name] = pipeline
        logger.info(f"创建处理管道: {pipeline.name}")
        return True
    
    def remove_pipeline(self, name: str) -> bool:
        """移除数据处理管道"""
        if name in self.pipelines:
            del self.pipelines[name]
            logger.info(f"移除处理管道: {name}")
            return True
        return False
    
    def process_data(self, algorithm_name: str, value: float, **kwargs) -> Optional[ProcessingResult]:
        """使用指定算法处理数据"""
        if algorithm_name not in self.algorithms:
            logger.error(f"算法 {algorithm_name} 不存在")
            return None
        
        config = self.algorithm_configs[algorithm_name]
        if not config.enabled:
            logger.debug(f"算法 {algorithm_name} 已禁用")
            return None
        
        try:
            algorithm = self.algorithms[algorithm_name]
            
            # 根据算法类型调用不同的处理方法
            if hasattr(algorithm, 'process'):
                if config.algorithm_type == 'trend_analyzer':
                    result = algorithm.process(value, kwargs.get('timestamp'))
                elif config.algorithm_type == 'statistical_analyzer':
                    # 统计分析器返回字典，需要包装成ProcessingResult
                    stats = algorithm.process(value)
                    result = ProcessingResult(
                        original_value=value,
                        processed_value=value,
                        confidence=1.0,
                        metadata=stats
                    )
                else:
                    result = algorithm.process(value)
                
                # 缓存结果
                self.results_cache[algorithm_name].append(result)
                if len(self.results_cache[algorithm_name]) > 1000:
                    self.results_cache[algorithm_name].pop(0)
                
                # 存储到数据库（如果启用）
                if self.db_manager:
                    try:
                        timestamp = kwargs.get('timestamp', time.time())
                        self.db_manager.store_algorithm_result(
                            timestamp=timestamp,
                            algorithm_name=algorithm_name,
                            module=kwargs.get('module', 'unknown'),
                            data_field=kwargs.get('data_field', 'value'),
                            original_value=value,
                            processed_value=result.processed_value,
                            confidence=result.confidence,
                            metadata=result.metadata or {}
                        )
                    except Exception as e:
                        logger.error(f"存储算法结果到数据库失败: {e}")
                
                return result
            else:
                logger.error(f"算法 {algorithm_name} 没有process方法")
                return None
                
        except Exception as e:
            logger.error(f"处理数据时出错: {e}")
            return None
    
    def process_pipeline(self, pipeline_name: str, module: str, data: Dict[str, Any]) -> Dict[str, ProcessingResult]:
        """执行数据处理管道"""
        if pipeline_name not in self.pipelines:
            logger.error(f"管道 {pipeline_name} 不存在")
            return {}
        
        pipeline = self.pipelines[pipeline_name]
        if not pipeline.enabled:
            logger.debug(f"管道 {pipeline_name} 已禁用")
            return {}
        
        # 检查模块是否在管道的输入模块列表中
        if pipeline.input_modules and module not in pipeline.input_modules:
            return {}
        
        results = {}
        
        # 对每个数据字段应用管道中的算法
        for key, value in data.items():
            if isinstance(value, (int, float)):
                field_results = {}
                
                for algo_name in pipeline.algorithms:
                    result = self.process_data(algo_name, float(value), timestamp=time.time())
                    if result:
                        field_results[algo_name] = result
                        
                        # 存储到数据库（带模块和字段信息）
                        if self.db_manager:
                            try:
                                self.db_manager.store_algorithm_result(
                                    timestamp=time.time(),
                                    algorithm_name=algo_name,
                                    module=module,
                                    data_field=key,
                                    original_value=float(value),
                                    processed_value=result.processed_value,
                                    confidence=result.confidence,
                                    metadata=result.metadata or {}
                                )
                            except Exception as e:
                                logger.error(f"存储管道算法结果到数据库失败: {e}")
                
                if field_results:
                    results[key] = field_results
        
        return results
    
    def connect_to_sensor_collector(self) -> bool:
        """连接到传感器收集器"""
        try:
            self.sensor_collector = get_sensor_collector()
            
            # 注册数据回调
            self.sensor_collector.add_data_callback(self._on_sensor_data)
            
            # 连接传感器收集器
            if not self.sensor_collector.connected:
                return self.sensor_collector.connect()
            
            return True
            
        except Exception as e:
            logger.error(f"连接传感器收集器失败: {e}")
            return False
    
    def _on_sensor_data(self, module: str, data: Dict[str, Any]):
        """传感器数据回调"""
        try:
            # 对所有启用的管道执行处理
            for pipeline_name, pipeline in self.pipelines.items():
                if pipeline.enabled:
                    results = self.process_pipeline(pipeline_name, module, data)
                    if results:
                        logger.debug(f"管道 {pipeline_name} 处理 {module} 数据: {len(results)} 个字段")
                        
        except Exception as e:
            logger.error(f"处理传感器数据回调时出错: {e}")
    
    def get_algorithm_results(self, algorithm_name: str, count: int = 10) -> List[ProcessingResult]:
        """获取算法处理结果"""
        if algorithm_name in self.results_cache:
            return self.results_cache[algorithm_name][-count:]
        return []
    
    def get_algorithm_status(self) -> Dict[str, Any]:
        """获取算法状态"""
        status = {
            'total_algorithms': len(self.algorithms),
            'enabled_algorithms': sum(1 for config in self.algorithm_configs.values() if config.enabled),
            'total_pipelines': len(self.pipelines),
            'enabled_pipelines': sum(1 for pipeline in self.pipelines.values() if pipeline.enabled),
            'database_enabled': self.enable_database,
            'algorithms': {}
        }
        
        for name, config in self.algorithm_configs.items():
            status['algorithms'][name] = {
                'type': config.algorithm_type,
                'enabled': config.enabled,
                'priority': config.priority,
                'results_count': len(self.results_cache.get(name, []))
            }
        
        # 添加数据库信息
        if self.db_manager:
            try:
                db_info = self.db_manager.get_database_info()
                status['database_info'] = db_info
            except Exception as e:
                logger.error(f"获取数据库信息失败: {e}")
                status['database_info'] = {'error': str(e)}
        
        return status
    
    def export_config(self) -> Dict[str, Any]:
        """导出配置"""
        return {
            'algorithms': [asdict(config) for config in self.algorithm_configs.values()],
            'pipelines': [asdict(pipeline) for pipeline in self.pipelines.values()]
        }
    
    def import_config(self, config_data: Dict[str, Any]) -> bool:
        """导入配置"""
        try:
            # 清除现有配置
            self.algorithms.clear()
            self.algorithm_configs.clear()
            self.pipelines.clear()
            
            # 导入算法配置
            if 'algorithms' in config_data:
                for algo_data in config_data['algorithms']:
                    config = AlgorithmConfig(**algo_data)
                    self.add_algorithm_config(config)
            
            # 导入管道配置
            if 'pipelines' in config_data:
                for pipe_data in config_data['pipelines']:
                    pipeline = ProcessingPipeline(**pipe_data)
                    self.create_pipeline(pipeline)
            
            logger.info("配置导入完成")
            return True
            
        except Exception as e:
            logger.error(f"导入配置失败: {e}")
            return False
    
    def start(self) -> bool:
        """启动算法管理器"""
        if self.running:
            return True
        
        # 连接传感器收集器
        if not self.connect_to_sensor_collector():
            logger.error("无法连接传感器收集器")
            return False
        
        self.running = True
        logger.info("算法管理器已启动")
        return True
    
    def stop(self):
        """停止算法管理器"""
        if not self.running:
            return
        
        self.running = False
        
        # 断开传感器收集器
        if self.sensor_collector:
            self.sensor_collector.remove_data_callback(self._on_sensor_data)
        
        logger.info("算法管理器已停止")

# 全局算法管理器实例
_algorithm_manager = None

def get_algorithm_manager() -> AlgorithmManager:
    """获取全局算法管理器实例"""
    global _algorithm_manager
    if _algorithm_manager is None:
        _algorithm_manager = AlgorithmManager()
    return _algorithm_manager 