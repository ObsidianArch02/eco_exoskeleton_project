"""
数据库功能演示脚本

演示数据库管理器的各种功能，包括数据存储、查询、统计和清理。
展示如何与传感器数据和算法结果进行交互。
"""

import time
import random
import json
from datetime import datetime, timedelta
from database_manager import get_database_manager

def generate_sensor_data():
    """生成模拟传感器数据"""
    modules = ['greenhouse', 'injection', 'bubble']
    
    sensor_data = {}
    for module in modules:
        if module == 'greenhouse':
            sensor_data[module] = {
                'temperature': round(random.uniform(20, 35), 2),
                'humidity': round(random.uniform(40, 80), 2),
                'deployed': random.choice([True, False]),
                'retracted': random.choice([True, False])
            }
        elif module == 'injection':
            sensor_data[module] = {
                'depth': round(random.uniform(0, 50), 2),
                'pressure': round(random.uniform(10, 100), 2),
                'needle_position': random.choice([True, False])
            }
        else:  # bubble
            sensor_data[module] = {
                'flow_rate': round(random.uniform(0, 10), 2),
                'tank_level': round(random.uniform(0, 100), 2),
                'system_pressure': round(random.uniform(20, 80), 2)
            }
    
    return sensor_data

def generate_algorithm_results():
    """生成模拟算法结果"""
    algorithms = ['moving_average', 'kalman_filter', 'outlier_detector', 'trend_analyzer']
    modules = ['greenhouse', 'injection', 'bubble']
    data_fields = ['temperature', 'humidity', 'depth', 'pressure', 'flow_rate']
    
    results = []
    for _ in range(random.randint(5, 15)):
        original_value = random.uniform(10, 100)
        processed_value = original_value + random.uniform(-5, 5)
        
        result = {
            'algorithm_name': random.choice(algorithms),
            'module': random.choice(modules),
            'data_field': random.choice(data_fields),
            'original_value': round(original_value, 2),
            'processed_value': round(processed_value, 2),
            'confidence': round(random.uniform(0.7, 1.0), 3),
            'metadata': {
                'processing_time': round(random.uniform(0.001, 0.1), 3),
                'algorithm_version': '1.0',
                'noise_level': round(random.uniform(0, 0.5), 2)
            }
        }
        results.append(result)
    
    return results

def demo_data_storage():
    """演示数据存储功能"""
    print("=" * 60)
    print("📝 数据存储演示")
    print("=" * 60)
    
    db_manager = get_database_manager()
    
    # 生成并存储传感器数据
    print("🔄 存储传感器数据...")
    sensor_data = generate_sensor_data()
    
    for module, data in sensor_data.items():
        success = db_manager.store_sensor_data(module, data)
        if success:
            print(f"✅ {module} 模块数据存储成功: {data}")
        else:
            print(f"❌ {module} 模块数据存储失败")
    
    print()
    
    # 生成并存储算法结果
    print("🔄 存储算法结果...")
    algorithm_results = generate_algorithm_results()
    
    for result in algorithm_results[:3]:  # 只显示前3个结果
        success = db_manager.store_algorithm_result(
            timestamp=time.time(),
            algorithm_name=result['algorithm_name'],
            module=result['module'],
            data_field=result['data_field'],
            original_value=result['original_value'],
            processed_value=result['processed_value'],
            confidence=result['confidence'],
            metadata=result['metadata']
        )
        
        if success:
            print(f"✅ 算法结果存储成功: {result['algorithm_name']} -> {result['module']}.{result['data_field']}")
        else:
            print(f"❌ 算法结果存储失败")
    
    print(f"📊 总共存储了 {len(algorithm_results)} 个算法结果")

def demo_data_query():
    """演示数据查询功能"""
    print("\n" + "=" * 60)
    print("🔍 数据查询演示")
    print("=" * 60)
    
    db_manager = get_database_manager()
    
    # 查询最新传感器数据
    print("📊 最新传感器数据 (最近10条):")
    sensor_data = db_manager.get_sensor_data(limit=10)
    
    for i, data in enumerate(sensor_data[:5]):  # 只显示前5条
        timestamp = datetime.fromtimestamp(data['timestamp']).strftime('%H:%M:%S')
        print(f"  {i+1}. [{timestamp}] {data['module']}.{data['data_type']}: {data['value']}")
    
    if len(sensor_data) > 5:
        print(f"  ... 还有 {len(sensor_data) - 5} 条记录")
    
    print()
    
    # 查询特定模块数据
    print("🏠 Greenhouse 模块数据:")
    greenhouse_data = db_manager.get_sensor_data(module='greenhouse', limit=5)
    
    for data in greenhouse_data:
        timestamp = datetime.fromtimestamp(data['timestamp']).strftime('%H:%M:%S')
        if data['data_type'] != 'raw':
            print(f"  [{timestamp}] {data['data_type']}: {data['value']}")
    
    print()
    
    # 查询算法结果
    print("🧠 最新算法结果:")
    algorithm_results = db_manager.get_algorithm_results(limit=5)
    
    for result in algorithm_results:
        timestamp = datetime.fromtimestamp(result['timestamp']).strftime('%H:%M:%S')
        print(f"  [{timestamp}] {result['algorithm_name']}: {result['original_value']} → {result['processed_value']} (置信度: {result['confidence']})")

def demo_statistics():
    """演示统计功能"""
    print("\n" + "=" * 60)
    print("📈 统计信息演示")
    print("=" * 60)
    
    db_manager = get_database_manager()
    
    # 获取数据库基本信息
    print("💾 数据库基本信息:")
    info = db_manager.get_database_info()
    
    print(f"  数据库路径: {info.get('database_path', 'N/A')}")
    print(f"  文件大小: {info.get('file_size_mb', 0):.2f} MB")
    print(f"  传感器数据记录: {info.get('sensor_data_count', 0):,}")
    print(f"  算法结果记录: {info.get('algorithm_results_count', 0):,}")
    print(f"  系统状态记录: {info.get('system_status_count', 0):,}")
    
    print()
    
    # 获取最近1小时的统计
    print("📊 最近1小时统计:")
    stats = db_manager.get_statistics(hours_back=1)
    
    if 'sensor_data' in stats:
        print("  传感器数据:")
        for module, module_stats in stats['sensor_data'].items():
            print(f"    {module} 模块:")
            for data_type, type_stats in module_stats.items():
                if data_type != 'raw':
                    count = type_stats.get('count', 0)
                    avg = type_stats.get('avg', 'N/A')
                    print(f"      {data_type}: {count} 条记录, 平均值: {avg}")
    
    if 'algorithm_results' in stats:
        print("  算法处理:")
        for algo_name, algo_stats in stats['algorithm_results'].items():
            count = algo_stats.get('count', 0)
            confidence = algo_stats.get('avg_confidence', 'N/A')
            print(f"    {algo_name}: {count} 次处理, 平均置信度: {confidence}")

def demo_time_range_query():
    """演示时间范围查询"""
    print("\n" + "=" * 60)
    print("⏰ 时间范围查询演示")
    print("=" * 60)
    
    db_manager = get_database_manager()
    
    # 查询最近5分钟的数据
    end_time = time.time()
    start_time = end_time - 300  # 5分钟前
    
    print("📅 查询最近5分钟的传感器数据:")
    recent_data = db_manager.get_sensor_data(
        start_time=start_time,
        end_time=end_time,
        limit=10
    )
    
    if recent_data:
        print(f"  找到 {len(recent_data)} 条记录")
        for data in recent_data[:3]:
            timestamp = datetime.fromtimestamp(data['timestamp']).strftime('%H:%M:%S')
            print(f"  [{timestamp}] {data['module']}.{data['data_type']}: {data['value']}")
    else:
        print("  📭 暂无数据")
    
    print()
    
    # 查询特定算法的结果
    print("🔬 查询移动平均算法结果:")
    ma_results = db_manager.get_algorithm_results(
        algorithm_name='moving_average',
        limit=5
    )
    
    if ma_results:
        print(f"  找到 {len(ma_results)} 条结果")
        for result in ma_results:
            timestamp = datetime.fromtimestamp(result['timestamp']).strftime('%H:%M:%S')
            print(f"  [{timestamp}] {result['module']}.{result['data_field']}: {result['original_value']} → {result['processed_value']}")
    else:
        print("  📭 暂无结果")

def main():
    """主演示函数"""
    print("🚀 数据库管理系统演示")
    print("这个演示将展示数据库的各种功能")
    print()
    
    try:
        # 演示数据存储
        demo_data_storage()
        
        # 演示数据查询
        demo_data_query()
        
        # 演示统计功能
        demo_statistics()
        
        # 演示时间范围查询
        demo_time_range_query()
        
        print("\n" + "=" * 60)
        print("✅ 数据库演示完成!")
        print("=" * 60)
        print("💡 提示:")
        print("- 运行 'python main.py' 启动系统")
        print("- 使用 CLI 命令: database, db_stats, db_cleanup")
        print("- 数据库文件: sensor_data.db")
        print("=" * 60)
        
    except Exception as e:
        print(f"❌ 演示过程中发生错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main() 