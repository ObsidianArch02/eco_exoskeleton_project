import time
from core.system_controller import EcologicalExoskeletonSystem
from models import ModuleState

class SystemCLI:
    def __init__(self, system: EcologicalExoskeletonSystem):
        self.system = system
        self.running = False
    
    def start(self):
        self.running = True
        print("生态修复外骨骼系统控制台")
        print("命令: start, stop, status, emergency, exit")
        
        while self.running:
            try:
                cmd = input("> ").strip().lower()
                
                if cmd == "start":
                    if self.system.start():
                        print("系统启动成功")
                    else:
                        print("系统启动失败")
                
                elif cmd == "stop":
                    self.system.stop()
                    print("系统已停止")
                
                elif cmd == "status":
                    self._show_system_status()
                
                elif cmd == "emergency":
                    self.system.emergency_stop()
                    print("紧急停止已执行")
                
                elif cmd in ["exit", "quit"]:
                    self.running = False
                    self.system.stop()
                    print("退出系统")
                
                else:
                    print("未知命令")
                    
            except KeyboardInterrupt:
                self.running = False
                self.system.stop()
                print("\n系统已安全关闭")
    
    def _show_system_status(self):
        if not hasattr(self.system, 'decision_system'):
            print("系统未初始化")
            return
            
        ds = self.system.decision_system
        print("\n===== 系统状态 =====")
        print(f"环境温度: {ds.environment.temperature:.1f}°C")
        print(f"环境湿度: {ds.environment.humidity:.1f}%")
        print(f"土壤湿度: {ds.environment.soil_moisture:.1f}%")
        
        print("\n===== 模块状态 =====")
        for module, status in ds.module_states.items():
            print(f"{module.upper()}: {status.state.value} - {status.message}")
        
        print("===================\n")
