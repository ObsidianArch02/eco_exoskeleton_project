import time
from system_controller import EcologicalExoskeletonSystem
from models import ModuleState

class SystemCLI:
    def __init__(self, system: EcologicalExoskeletonSystem):
        self.system = system
        self.running = False
    
    def start(self):
        self.running = True
        print("生态修复外骨骼系统控制台 (增强版)")
        print("=" * 50)
        print("基础命令: start, stop, status, emergency, exit")
        print("数据处理: algorithms, sensor_data, processed_data, pipelines")
        print("=" * 50)
        
        while self.running:
            try:
                cmd = input("> ").strip().lower()
                
                if cmd == "start":
                    if self.system.start():
                        print("✅ 系统启动成功")
                    else:
                        print("❌ 系统启动失败")
                
                elif cmd == "stop":
                    self.system.stop()
                    print("✅ 系统已停止")
                
                elif cmd == "status":
                    self._show_system_status()
                
                elif cmd == "emergency":
                    self.system.emergency_stop()
                    print("🚨 紧急停止已执行")
                
                elif cmd == "algorithms":
                    self._show_algorithm_status()
                
                elif cmd == "sensor_data":
                    self._show_sensor_data()
                
                elif cmd == "processed_data":
                    self._show_processed_data()
                
                elif cmd == "pipelines":
                    self._show_pipelines()
                
                elif cmd in ["exit", "quit"]:
                    self.running = False
                    self.system.stop()
                    print("👋 退出系统")
                
                elif cmd == "help":
                    self._show_help()
                
                else:
                    print("❓ 未知命令，输入 'help' 查看帮助")
                    
            except KeyboardInterrupt:
                self.running = False
                self.system.stop()
                print("\n🛑 系统已安全关闭")
    
    def _show_system_status(self):
        """显示系统状态"""
        print("\n" + "=" * 60)
        print("🖥️  系统状态")
        print("=" * 60)
        
        try:
            status = self.system.get_system_status()
            
            # 基础系统状态
            print(f"系统运行状态: {'🟢 运行中' if status['system_running'] else '🔴 已停止'}")
            print(f"MQTT连接状态: {'🟢 已连接' if status['mqtt_connected'] else '🔴 未连接'}")
            
            # 传感器收集器状态
            sensor_status = status['sensor_collector']
            print(f"\n📊 传感器收集器:")
            print(f"  连接状态: {'🟢 已连接' if sensor_status['connected'] else '🔴 未连接'}")
            print(f"  总数据条数: {sensor_status['total_entries']}")
            print(f"  缓冲区大小: {sensor_status['buffer_size']}")
            
            # 算法管理器状态
            algo_status = status['algorithm_manager']
            print(f"\n🧠 算法管理器:")
            print(f"  总算法数: {algo_status['total_algorithms']}")
            print(f"  启用算法数: {algo_status['enabled_algorithms']}")
            print(f"  总管道数: {algo_status['total_pipelines']}")
            print(f"  启用管道数: {algo_status['enabled_pipelines']}")
            
            # 决策系统状态
            decision_status = status['decision_system']
            print(f"\n🎯 决策系统:")
            print(f"  修复计划长度: {decision_status['repair_plan_length']}")
            
            # 模块状态
            print(f"\n🏭 模块状态:")
            for module, state in decision_status['module_states'].items():
                print(f"  {module.upper()}: {state}")
                
        except Exception as e:
            print(f"❌ 获取系统状态失败: {e}")
        
        print("=" * 60)
    
    def _show_algorithm_status(self):
        """显示算法状态"""
        print("\n" + "=" * 60)
        print("🧠 算法状态")
        print("=" * 60)
        
        try:
            algo_manager = self.system.algorithm_manager
            status = algo_manager.get_algorithm_status()
            
            print(f"总算法数: {status['total_algorithms']}")
            print(f"启用算法数: {status['enabled_algorithms']}")
            
            print("\n算法详情:")
            print("-" * 60)
            for name, info in status['algorithms'].items():
                enabled_icon = "🟢" if info['enabled'] else "🔴"
                print(f"{enabled_icon} {name}")
                print(f"    类型: {info['type']}")
                print(f"    优先级: {info['priority']}")
                print(f"    结果数量: {info['results_count']}")
                print()
                
        except Exception as e:
            print(f"❌ 获取算法状态失败: {e}")
        
        print("=" * 60)
    
    def _show_sensor_data(self):
        """显示传感器数据"""
        print("\n" + "=" * 60)
        print("📊 传感器数据")
        print("=" * 60)
        
        try:
            collector = self.system.sensor_collector
            
            for module in ['greenhouse', 'injection', 'bubble']:
                print(f"\n🏭 {module.upper()} 模块:")
                latest_data = collector.get_latest_data(module)
                
                if latest_data:
                    print(f"  时间戳: {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(latest_data['timestamp']))}")
                    print(f"  数据:")
                    for key, value in latest_data['data'].items():
                        if isinstance(value, (int, float)):
                            print(f"    {key}: {value:.2f}")
                        else:
                            print(f"    {key}: {value}")
                else:
                    print("  🔴 暂无数据")
                
        except Exception as e:
            print(f"❌ 获取传感器数据失败: {e}")
        
        print("=" * 60)
    
    def _show_processed_data(self):
        """显示处理后的数据"""
        print("\n" + "=" * 60)
        print("⚙️  处理后的数据")
        print("=" * 60)
        
        try:
            summary = self.system.get_processed_data_summary()
            
            # 显示原始数据
            print("📊 最新原始数据:")
            for module, data in summary.items():
                if module != 'algorithm_results':
                    print(f"\n🏭 {module.upper()}:")
                    print(f"  时间戳: {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(data['timestamp']))}")
                    for key, value in data['raw_data'].items():
                        if isinstance(value, (int, float)):
                            print(f"  {key}: {value:.2f}")
                        else:
                            print(f"  {key}: {value}")
            
            # 显示算法处理结果
            print(f"\n🧠 算法处理结果:")
            if 'algorithm_results' in summary:
                for algo_name, result in summary['algorithm_results'].items():
                    confidence_icon = "🟢" if result['confidence'] > 0.8 else "🟡" if result['confidence'] > 0.5 else "🔴"
                    print(f"  {confidence_icon} {algo_name}:")
                    print(f"    处理值: {result['processed_value']:.3f}")
                    print(f"    置信度: {result['confidence']:.3f}")
                    print(f"    算法: {result['algorithm']}")
            else:
                print("  🔴 暂无算法结果")
                
        except Exception as e:
            print(f"❌ 获取处理数据失败: {e}")
        
        print("=" * 60)
    
    def _show_pipelines(self):
        """显示数据处理管道"""
        print("\n" + "=" * 60)
        print("🔄 数据处理管道")
        print("=" * 60)
        
        try:
            algo_manager = self.system.algorithm_manager
            
            if not algo_manager.pipelines:
                print("🔴 暂无数据处理管道")
                return
            
            for name, pipeline in algo_manager.pipelines.items():
                enabled_icon = "🟢" if pipeline.enabled else "🔴"
                print(f"{enabled_icon} {name}")
                print(f"    输入模块: {', '.join(pipeline.input_modules) if pipeline.input_modules else '所有模块'}")
                print(f"    算法链: {' → '.join(pipeline.algorithms)}")
                print()
                
        except Exception as e:
            print(f"❌ 获取管道信息失败: {e}")
        
        print("=" * 60)
    
    def _show_help(self):
        """显示帮助信息"""
        print("\n" + "=" * 60)
        print("📖 命令帮助")
        print("=" * 60)
        print("基础命令:")
        print("  start          - 启动系统")
        print("  stop           - 停止系统") 
        print("  status         - 显示系统状态")
        print("  emergency      - 紧急停止")
        print("  exit/quit      - 退出程序")
        print()
        print("数据处理命令:")
        print("  algorithms     - 显示算法状态")
        print("  sensor_data    - 显示传感器数据")
        print("  processed_data - 显示处理后的数据")
        print("  pipelines      - 显示数据处理管道")
        print("  help           - 显示此帮助信息")
        print("=" * 60)
