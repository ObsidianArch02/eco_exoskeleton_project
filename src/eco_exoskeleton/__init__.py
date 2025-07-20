"""
Eco-Exoskeleton System for Ecological Restoration

A distributed robotics platform for autonomous environmental restoration.
"""

__version__ = "0.1.0"
__author__ = "Eco-Exoskeleton Team"
__description__ = "Advanced robotics platform for ecological restoration"

# Core system components
from .system_controller import EcologicalExoskeletonSystem
from .decision_system import CentralDecisionSystem
from .mqtt_manager import MQTTManager
from .sensor_collector import SensorCollector, get_sensor_collector
from .algorithm_manager import AlgorithmManager, get_algorithm_manager
from .database_manager import DatabaseManager, get_database_manager
from .cli import SystemCLI

# Data models
from .models import SensorData, ModuleStatus, Command, ModuleState

# Configuration
from .config import *

__all__ = [
    # Core classes
    "EcologicalExoskeletonSystem",
    "CentralDecisionSystem", 
    "MQTTManager",
    "SensorCollector",
    "AlgorithmManager",
    "DatabaseManager",
    "SystemCLI",
    
    # Data models
    "SensorData",
    "ModuleStatus",
    "Command", 
    "ModuleState",
    
    # Factory functions
    "get_sensor_collector",
    "get_algorithm_manager",
    "get_database_manager",
]
