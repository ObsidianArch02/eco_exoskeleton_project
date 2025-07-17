"""
算法演示脚本

演示各种数据处理算法的功能，提供测试数据和结果展示。
用于测试和验证算法的正确性和性能。
"""

import time
import random
import math
from typing import List, Tuple
from data_processing import (
    MovingAverageFilter, KalmanFilter, OutlierDetector, 
    TrendAnalyzer, StatisticalAnalyzer, DataFusionProcessor, AdaptiveFilter
)
from algorithm_manager import get_algorithm_manager, AlgorithmConfig, ProcessingPipeline
from sensor_collector import get_sensor_collector
from log_manager import setup_logging

def generate_test_data(length: int = 100) -> List[Tuple[float, float]]:
    """生成测试数据 (时间戳, 数值)"""
    data = []
    base_time = time.time()
    
    for i in range(length):
        # 生成带噪声和趋势的数据
        timestamp = base_time + i
        trend = 0.1 * i  # 线性增长趋势
        noise = random.gauss(0, 0.5)  # 高斯噪声
        seasonal = 2 * math.sin(2 * math.pi * i / 20)  # 季节性变化
        
        # 添加一些异常值
        if random.random() < 0.05:  # 5%的异常值概率
            outlier = random.choice([5, -5])
            value = 25 + trend + seasonal + noise + outlier
        else:
            value = 25 + trend + seasonal + noise
        
        data.append((timestamp, value))
    
    return data

def demo_moving_average():
    """演示移动平均滤波器"""
    print("\n" + "=" * 60)
    print("🎯 移动平均滤波器演示")
    print("=" * 60)
    
    filter_3 = MovingAverageFilter(window_size=3)
    filter_10 = MovingAverageFilter(window_size=10)
    
    test_data = generate_test_data(20)
    
    print("时间戳\t\t原始值\t窗口3\t窗口10")
    print("-" * 60)
    
    for timestamp, value in test_data:
        result_3 = filter_3.process(value)
        result_10 = filter_10.process(value)
        
        time_str = time.strftime('%H:%M:%S', time.localtime(timestamp))
        print(f"{time_str}\t{value:.2f}\t{result_3.processed_value:.2f}\t{result_10.processed_value:.2f}")

def demo_kalman_filter():
    """演示卡尔曼滤波器"""
    print("\n" + "=" * 60)
    print("🎯 卡尔曼滤波器演示")
    print("=" * 60)
    
    kalman = KalmanFilter(process_variance=1e-5, measurement_variance=1e-1)
    test_data = generate_test_data(15)
    
    print("时间戳\t\t原始值\t卡尔曼值\t置信度")
    print("-" * 60)
    
    for timestamp, value in test_data:
        result = kalman.process(value)
        time_str = time.strftime('%H:%M:%S', time.localtime(timestamp))
        print(f"{time_str}\t{value:.2f}\t{result.processed_value:.2f}\t\t{result.confidence:.3f}")

def demo_outlier_detection():
    """演示异常值检测"""
    print("\n" + "=" * 60)
    print("🎯 异常值检测演示")
    print("=" * 60)
    
    detector = OutlierDetector(window_size=10, threshold_multiplier=2.0)
    test_data = generate_test_data(25)
    
    print("时间戳\t\t原始值\t处理值\t异常检测\tZ分数")
    print("-" * 70)
    
    for timestamp, value in test_data:
        result = detector.process(value)
        time_str = time.strftime('%H:%M:%S', time.localtime(timestamp))
        is_outlier = result.metadata.get('is_outlier', False)
        z_score = result.metadata.get('z_score', 0)
        outlier_str = "🔴异常" if is_outlier else "🟢正常"
        
        print(f"{time_str}\t{value:.2f}\t{result.processed_value:.2f}\t{outlier_str}\t\t{z_score:.2f}")

def demo_trend_analysis():
    """演示趋势分析"""
    print("\n" + "=" * 60)
    print("🎯 趋势分析演示")
    print("=" * 60)
    
    analyzer = TrendAnalyzer(window_size=8)
    test_data = generate_test_data(20)
    
    print("时间戳\t\t数值\t趋势\t\t斜率\t\tR²")
    print("-" * 70)
    
    for timestamp, value in test_data:
        result = analyzer.process(value, timestamp)
        time_str = time.strftime('%H:%M:%S', time.localtime(timestamp))
        trend = result.metadata.get('trend', 'unknown')
        slope = result.metadata.get('slope', 0)
        r_squared = result.metadata.get('r_squared', 0)
        
        trend_icon = {"increasing": "📈", "decreasing": "📉", "stable": "➡️"}.get(trend, "❓")
        
        print(f"{time_str}\t{value:.2f}\t{trend_icon}{trend}\t{slope:.4f}\t\t{r_squared:.3f}")

def demo_data_fusion():
    """演示数据融合"""
    print("\n" + "=" * 60)
    print("🎯 数据融合演示")
    print("=" * 60)
    
    fusion = DataFusionProcessor()
    
    # 设置传感器权重和可靠性
    fusion.set_sensor_weight("sensor_a", 0.8)
    fusion.set_sensor_weight("sensor_b", 0.6)
    fusion.set_sensor_weight("sensor_c", 0.9)
    
    fusion.update_sensor_reliability("sensor_a", 0.9)
    fusion.update_sensor_reliability("sensor_b", 0.7)
    fusion.update_sensor_reliability("sensor_c", 0.95)
    
    print("传感器A\t传感器B\t传感器C\t融合值\t置信度")
    print("-" * 60)
    
    for i in range(10):
        # 模拟三个传感器的读数
        base_value = 25 + random.gauss(0, 0.5)
        sensor_data = {
            "sensor_a": base_value + random.gauss(0, 0.2),
            "sensor_b": base_value + random.gauss(0, 0.3),
            "sensor_c": base_value + random.gauss(0, 0.1)
        }
        
        result = fusion.fuse_data(sensor_data)
        
        print(f"{sensor_data['sensor_a']:.2f}\t{sensor_data['sensor_b']:.2f}\t"
              f"{sensor_data['sensor_c']:.2f}\t{result.processed_value:.2f}\t{result.confidence:.3f}")

def demo_adaptive_filter():
    """演示自适应滤波器"""
    print("\n" + "=" * 60)
    print("🎯 自适应滤波器演示")
    print("=" * 60)
    
    adaptive = AdaptiveFilter(initial_alpha=0.1)
    test_data = generate_test_data(20)
    
    print("时间戳\t\t原始值\t滤波值\t学习率\t置信度")
    print("-" * 70)
    
    for timestamp, value in test_data:
        result = adaptive.process(value)
        time_str = time.strftime('%H:%M:%S', time.localtime(timestamp))
        alpha = result.metadata.get('alpha', 0)
        
        print(f"{time_str}\t{value:.2f}\t{result.processed_value:.2f}\t{alpha:.3f}\t\t{result.confidence:.3f}")

def demo_algorithm_manager():
    """演示算法管理器"""
    print("\n" + "=" * 60)
    print("🎯 算法管理器演示")
    print("=" * 60)
    
    # 获取算法管理器
    manager = get_algorithm_manager()
    
    # 显示算法状态
    status = manager.get_algorithm_status()
    print(f"算法管理器状态:")
    print(f"  总算法数: {status['total_algorithms']}")
    print(f"  启用算法数: {status['enabled_algorithms']}")
    
    # 创建测试管道
    test_pipeline = ProcessingPipeline(
        name="测试管道",
        algorithms=["温度滤波", "异常检测"],
        input_modules=["test_module"]
    )
    
    manager.create_pipeline(test_pipeline)
    
    # 模拟传感器数据处理
    print("\n处理测试数据:")
    test_sensor_data = {
        "temperature": 25.5,
        "humidity": 60.2,
        "pressure": 1013.25
    }
    
    pipeline_results = manager.process_pipeline("测试管道", "test_module", test_sensor_data)
    
    if pipeline_results:
        for field, algo_results in pipeline_results.items():
            print(f"\n{field} 字段处理结果:")
            for algo_name, result in algo_results.items():
                print(f"  {algo_name}: {result.original_value:.2f} → {result.processed_value:.2f} (置信度: {result.confidence:.3f})")
    else:
        print("  暂无处理结果")

def demo_statistical_analysis():
    """演示统计分析"""
    print("\n" + "=" * 60)
    print("🎯 统计分析演示")
    print("=" * 60)
    
    analyzer = StatisticalAnalyzer(window_size=20)
    test_data = generate_test_data(30)
    
    print("添加数据点后的统计信息:")
    print("-" * 60)
    
    for i, (timestamp, value) in enumerate(test_data[:15]):
        stats = analyzer.process(value)
        
        if i % 5 == 4:  # 每5个数据点显示一次统计
            print(f"\n第 {i+1} 个数据点后:")
            print(f"  样本数: {stats['count']}")
            print(f"  均值: {stats['mean']:.2f}")
            print(f"  中位数: {stats['median']:.2f}")
            print(f"  标准差: {stats['std_dev']:.3f}")
            print(f"  最小值: {stats['min']:.2f}")
            print(f"  最大值: {stats['max']:.2f}")
            print(f"  范围: {stats['range']:.2f}")

def main():
    """主演示函数"""
    setup_logging(level=20)  # INFO级别
    
    print("🧠 数据处理算法演示系统")
    print("=" * 60)
    print("本演示将展示各种数据处理算法的功能")
    print("=" * 60)
    
    demos = [
        ("移动平均滤波器", demo_moving_average),
        ("卡尔曼滤波器", demo_kalman_filter),
        ("异常值检测", demo_outlier_detection),
        ("趋势分析", demo_trend_analysis),
        ("数据融合", demo_data_fusion),
        ("自适应滤波器", demo_adaptive_filter),
        ("统计分析", demo_statistical_analysis),
        ("算法管理器", demo_algorithm_manager)
    ]
    
    while True:
        print(f"\n🎯 选择要演示的算法:")
        for i, (name, _) in enumerate(demos, 1):
            print(f"  {i}. {name}")
        print(f"  {len(demos) + 1}. 运行所有演示")
        print(f"  0. 退出")
        
        try:
            choice = input("\n请选择 (0-{}): ".format(len(demos) + 1)).strip()
            
            if choice == "0":
                print("👋 演示结束")
                break
            elif choice == str(len(demos) + 1):
                print("\n🚀 运行所有演示...")
                for name, demo_func in demos:
                    print(f"\n▶️  开始演示: {name}")
                    demo_func()
                    input("\n按回车键继续下一个演示...")
            elif choice.isdigit() and 1 <= int(choice) <= len(demos):
                name, demo_func = demos[int(choice) - 1]
                print(f"\n▶️  开始演示: {name}")
                demo_func()
            else:
                print("❌ 无效选择，请重试")
                
        except KeyboardInterrupt:
            print("\n👋 演示结束")
            break
        except Exception as e:
            print(f"❌ 演示过程中出错: {e}")

if __name__ == "__main__":
    main() 