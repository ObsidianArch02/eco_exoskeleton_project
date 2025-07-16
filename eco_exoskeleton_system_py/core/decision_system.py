import time
import logging
from typing import Dict, List, Optional
from ..models import SensorData, ModuleStatus, Command, ModuleState
from ..config import DECISION_INTERVAL

logger = logging.getLogger(__name__)

class CentralDecisionSystem:
    def __init__(self):
        self.environment = SensorData()
        self.module_states: Dict[str, ModuleStatus] = {
            "greenhouse": ModuleStatus("greenhouse", ModuleState.IDLE, "初始化", time.time()),
            "injection": ModuleStatus("injection", ModuleState.IDLE, "初始化", time.time()),
            "bubble": ModuleStatus("bubble", ModuleState.IDLE, "初始化", time.time())
        }
        self.repair_plan: List[Command] = []
        self.last_decision_time = time.time()
        
    def update_sensor_data(self, sensor_data: SensorData):
        self.environment = sensor_data
        if not self.repair_plan and self.environment.damage_areas:
            self._generate_repair_plan()
    
    def update_module_status(self, status: ModuleStatus):
        self.module_states[status.module] = status
        if status.state == ModuleState.COMPLETED:
            self._handle_task_completion(status.module)
        elif status.state == ModuleState.ERROR:
            self._handle_module_error(status.module)
    
    def make_decision(self) -> Optional[Command]:
        current_time = time.time()
        if current_time - self.last_decision_time < DECISION_INTERVAL:
            return None

        self.last_decision_time = current_time

        if self.repair_plan:
            return self.repair_plan.pop(0)

        return self._monitor_environment()
    
    def _generate_repair_plan(self):
        if not self.environment.damage_areas:
            return
            
        self.repair_plan = [
            Command("greenhouse", "deploy", {"location": self.environment.damage_areas[0]}),
            Command("injection", "inject", {
                "depth": 15, 
                "pressure": 200,
                "location": self.environment.damage_areas[0]
            }),
            Command("bubble", "spray", {
                "duration": 3000,
                "intensity": 80,
                "location": self.environment.damage_areas[0]
            }),
            Command("greenhouse", "retract", {})
        ]
    
    def _handle_task_completion(self, module: str):
        logger.info(f"{module} 模块任务完成，继续执行后续计划")
    
    def _handle_module_error(self, module: str):
        error_status = self.module_states[module]
        if "超时" in error_status.message:
            logger.warning(f"{module} 模块超时，尝试重试...")
            if module == "greenhouse":
                return Command(module, "deploy", {})
            elif module == "injection":
                return Command(module, "inject", {"depth": 10, "pressure": 150})
            elif module == "bubble":
                return Command(module, "spray", {"duration": 3000, "intensity": 80})
        logger.error(f"{module} 模块错误: {error_status.message}")
        return None
    
    def _monitor_environment(self) -> Optional[Command]:
        if self.environment.temperature < 10 and self.module_states["greenhouse"].state == ModuleState.IDLE:
            return Command("greenhouse", "deploy", {})
            
        if self.environment.soil_moisture < 30 and self.module_states["injection"].state == ModuleState.IDLE:
            return Command("injection", "inject", {"depth": 10, "pressure": 150})
        
        return None
