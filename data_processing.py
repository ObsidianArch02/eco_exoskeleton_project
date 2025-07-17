"""
数据处理算法模块

包含各种传感器数据处理算法，如滤波、统计分析、异常检测、
数据融合、趋势分析等。提供可扩展的算法接口。
"""

import math
import statistics
from typing import List, Dict, Optional, Tuple, Any
from collections import deque
from dataclasses import dataclass
import logging

logger = logging.getLogger(__name__)

@dataclass
class ProcessingResult:
    """数据处理结果"""
    original_value: float
    processed_value: float
    confidence: float
    metadata: Dict[str, Any]

class MovingAverageFilter:
    """移动平均滤波器"""
    
    def __init__(self, window_size: int = 5):
        self.window_size = window_size
        self.data_buffer = deque(maxlen=window_size)
    
    def process(self, value: float) -> ProcessingResult:
        """处理单个数值"""
        self.data_buffer.append(value)
        
        if len(self.data_buffer) < self.window_size:
            # 数据不足时使用简单平均
            processed = sum(self.data_buffer) / len(self.data_buffer)
            confidence = len(self.data_buffer) / self.window_size
        else:
            # 使用完整窗口的移动平均
            processed = sum(self.data_buffer) / self.window_size
            confidence = 1.0
        
        return ProcessingResult(
            original_value=value,
            processed_value=processed,
            confidence=confidence,
            metadata={
                'algorithm': 'moving_average',
                'window_size': self.window_size,
                'samples_used': len(self.data_buffer)
            }
        )

class KalmanFilter:
    """简化的卡尔曼滤波器"""
    
    def __init__(self, process_variance: float = 1e-5, measurement_variance: float = 1e-1):
        self.process_variance = process_variance
        self.measurement_variance = measurement_variance
        self.estimated_value = None
        self.estimation_error = 1.0
    
    def process(self, measurement: float) -> ProcessingResult:
        """处理传感器测量值"""
        if self.estimated_value is None:
            # 初始化
            self.estimated_value = measurement
            confidence = 0.5
        else:
            # 预测步骤
            predicted_error = self.estimation_error + self.process_variance
            
            # 更新步骤
            kalman_gain = predicted_error / (predicted_error + self.measurement_variance)
            self.estimated_value = self.estimated_value + kalman_gain * (measurement - self.estimated_value)
            self.estimation_error = (1 - kalman_gain) * predicted_error
            
            # 计算置信度（基于误差减少量）
            confidence = 1.0 - min(self.estimation_error, 1.0)
        
        return ProcessingResult(
            original_value=measurement,
            processed_value=self.estimated_value,
            confidence=confidence,
            metadata={
                'algorithm': 'kalman_filter',
                'kalman_gain': kalman_gain if self.estimated_value != measurement else 0,
                'estimation_error': self.estimation_error
            }
        )

class OutlierDetector:
    """异常值检测器"""
    
    def __init__(self, window_size: int = 20, threshold_multiplier: float = 2.0):
        self.window_size = window_size
        self.threshold_multiplier = threshold_multiplier
        self.data_buffer = deque(maxlen=window_size)
    
    def process(self, value: float) -> ProcessingResult:
        """检测异常值"""
        self.data_buffer.append(value)
        
        if len(self.data_buffer) < 3:
            # 数据不足，认为是正常值
            return ProcessingResult(
                original_value=value,
                processed_value=value,
                confidence=0.5,
                metadata={
                    'algorithm': 'outlier_detection',
                    'is_outlier': False,
                    'reason': 'insufficient_data'
                }
            )
        
        # 计算统计信息
        mean = statistics.mean(self.data_buffer)
        std_dev = statistics.stdev(self.data_buffer) if len(self.data_buffer) > 1 else 0
        
        # 检测异常值（使用Z-score方法）
        if std_dev > 0:
            z_score = abs(value - mean) / std_dev
            is_outlier = z_score > self.threshold_multiplier
        else:
            is_outlier = False
            z_score = 0
        
        # 如果是异常值，使用中位数替代
        processed_value = value if not is_outlier else statistics.median(self.data_buffer)
        confidence = 1.0 - min(z_score / (self.threshold_multiplier * 2), 1.0)
        
        return ProcessingResult(
            original_value=value,
            processed_value=processed_value,
            confidence=confidence,
            metadata={
                'algorithm': 'outlier_detection',
                'is_outlier': is_outlier,
                'z_score': z_score,
                'mean': mean,
                'std_dev': std_dev
            }
        )

class TrendAnalyzer:
    """趋势分析器"""
    
    def __init__(self, window_size: int = 10):
        self.window_size = window_size
        self.data_buffer = deque(maxlen=window_size)
        self.time_buffer = deque(maxlen=window_size)
    
    def process(self, value: float, timestamp: Optional[float] = None) -> ProcessingResult:
        """分析数据趋势"""
        import time
        if timestamp is None:
            timestamp = time.time()
        
        self.data_buffer.append(value)
        self.time_buffer.append(timestamp)
        
        if len(self.data_buffer) < 3:
            return ProcessingResult(
                original_value=value,
                processed_value=value,
                confidence=0.3,
                metadata={
                    'algorithm': 'trend_analysis',
                    'trend': 'unknown',
                    'slope': 0,
                    'r_squared': 0
                }
            )
        
        # 计算线性回归
        n = len(self.data_buffer)
        x_values = list(range(n))
        y_values = list(self.data_buffer)
        
        # 计算斜率和截距
        x_mean = sum(x_values) / n
        y_mean = sum(y_values) / n
        
        numerator = sum((x_values[i] - x_mean) * (y_values[i] - y_mean) for i in range(n))
        denominator = sum((x_values[i] - x_mean) ** 2 for i in range(n))
        
        if denominator == 0:
            slope = 0
            r_squared = 0
        else:
            slope = numerator / denominator
            
            # 计算R²
            y_pred = [slope * (i - x_mean) + y_mean for i in range(n)]
            ss_res = sum((y_values[i] - y_pred[i]) ** 2 for i in range(n))
            ss_tot = sum((y_values[i] - y_mean) ** 2 for i in range(n))
            r_squared = 1 - (ss_res / ss_tot) if ss_tot != 0 else 0
        
        # 确定趋势
        if abs(slope) < 0.01:
            trend = 'stable'
        elif slope > 0:
            trend = 'increasing'
        else:
            trend = 'decreasing'
        
        confidence = min(r_squared, 1.0)
        
        return ProcessingResult(
            original_value=value,
            processed_value=value,  # 趋势分析不改变原值
            confidence=confidence,
            metadata={
                'algorithm': 'trend_analysis',
                'trend': trend,
                'slope': slope,
                'r_squared': r_squared,
                'sample_count': n
            }
        )

class StatisticalAnalyzer:
    """统计分析器"""
    
    def __init__(self, window_size: int = 50):
        self.window_size = window_size
        self.data_buffer = deque(maxlen=window_size)
    
    def process(self, value: float) -> Dict[str, Any]:
        """进行统计分析"""
        self.data_buffer.append(value)
        
        if len(self.data_buffer) < 2:
            return {
                'count': len(self.data_buffer),
                'mean': value,
                'median': value,
                'std_dev': 0,
                'min': value,
                'max': value,
                'range': 0
            }
        
        data_list = list(self.data_buffer)
        
        return {
            'count': len(data_list),
            'mean': statistics.mean(data_list),
            'median': statistics.median(data_list),
            'std_dev': statistics.stdev(data_list),
            'min': min(data_list),
            'max': max(data_list),
            'range': max(data_list) - min(data_list),
            'variance': statistics.variance(data_list)
        }

class DataFusionProcessor:
    """数据融合处理器"""
    
    def __init__(self):
        self.sensor_weights = {}
        self.sensor_reliability = {}
    
    def set_sensor_weight(self, sensor_name: str, weight: float):
        """设置传感器权重"""
        self.sensor_weights[sensor_name] = max(0.0, min(1.0, weight))
    
    def update_sensor_reliability(self, sensor_name: str, reliability: float):
        """更新传感器可靠性"""
        self.sensor_reliability[sensor_name] = max(0.0, min(1.0, reliability))
    
    def fuse_data(self, sensor_data: Dict[str, float]) -> ProcessingResult:
        """融合多传感器数据"""
        if not sensor_data:
            raise ValueError("传感器数据不能为空")
        
        # 如果只有一个传感器，直接返回
        if len(sensor_data) == 1:
            sensor_name, value = next(iter(sensor_data.items()))
            return ProcessingResult(
                original_value=value,
                processed_value=value,
                confidence=self.sensor_reliability.get(sensor_name, 0.8),
                metadata={
                    'algorithm': 'data_fusion',
                    'sensor_count': 1,
                    'primary_sensor': sensor_name
                }
            )
        
        # 加权平均融合
        total_weight = 0
        weighted_sum = 0
        
        for sensor_name, value in sensor_data.items():
            weight = self.sensor_weights.get(sensor_name, 1.0)
            reliability = self.sensor_reliability.get(sensor_name, 0.8)
            effective_weight = weight * reliability
            
            weighted_sum += value * effective_weight
            total_weight += effective_weight
        
        if total_weight == 0:
            # 所有权重为0，使用简单平均
            fused_value = sum(sensor_data.values()) / len(sensor_data)
            confidence = 0.5
        else:
            fused_value = weighted_sum / total_weight
            confidence = min(total_weight / len(sensor_data), 1.0)
        
        return ProcessingResult(
            original_value=list(sensor_data.values())[0],  # 使用第一个传感器作为原始值
            processed_value=fused_value,
            confidence=confidence,
            metadata={
                'algorithm': 'data_fusion',
                'sensor_count': len(sensor_data),
                'total_weight': total_weight,
                'sensors_used': list(sensor_data.keys())
            }
        )

class AdaptiveFilter:
    """自适应滤波器"""
    
    def __init__(self, initial_alpha: float = 0.1):
        self.alpha = initial_alpha  # 学习率
        self.filtered_value = None
        self.error_history = deque(maxlen=10)
        
    def process(self, value: float) -> ProcessingResult:
        """自适应滤波处理"""
        if self.filtered_value is None:
            self.filtered_value = value
            confidence = 0.5
        else:
            # 计算误差
            error = abs(value - self.filtered_value)
            self.error_history.append(error)
            
            # 自适应调整学习率
            if len(self.error_history) > 3:
                recent_avg_error = sum(list(self.error_history)[-3:]) / 3
                long_term_avg_error = sum(self.error_history) / len(self.error_history)
                
                if recent_avg_error > long_term_avg_error * 1.5:
                    # 误差增大，提高学习率
                    self.alpha = min(0.5, self.alpha * 1.1)
                else:
                    # 误差稳定，降低学习率
                    self.alpha = max(0.01, self.alpha * 0.95)
            
            # 指数移动平均滤波
            self.filtered_value = self.alpha * value + (1 - self.alpha) * self.filtered_value
            confidence = 1.0 - min(error / (abs(value) + 1e-6), 1.0)
        
        return ProcessingResult(
            original_value=value,
            processed_value=self.filtered_value,
            confidence=confidence,
            metadata={
                'algorithm': 'adaptive_filter',
                'alpha': self.alpha,
                'error_history_size': len(self.error_history)
            }
        ) 