from enum import Enum
from dataclasses import dataclass, field
from typing import List, Tuple

class ModuleState(Enum):
    IDLE = "IDLE"
    DEPLOYING = "DEPLOYING"
    RETRACTING = "RETRACTING"
    INJECTING = "INJECTING"
    SPRAYING = "SPRAYING"
    ERROR = "ERROR"
    COMPLETED = "COMPLETED"

@dataclass
class SensorData:
    temperature: float = 25.0
    humidity: float = 50.0
    soil_moisture: float = 0.0
    wind_speed: float = 0.0
    terrain_type: str = "unknown"
    damage_areas: List[Tuple[float, float]] = field(default_factory=list)
    injection_depth: float = 0.0
    bubble_flow: float = 0.0

@dataclass
class ModuleStatus:
    module: str
    state: ModuleState
    message: str
    timestamp: float
    data: dict = field(default_factory=dict)

@dataclass
class Command:
    module: str
    action: str
    params: dict
