"""
Test Sensor Generator for Eco-Exoskeleton System

This script generates random sensor data and feeds it to the CentralDecisionSystem
for testing purposes. Run this script standalone to simulate sensor input.
"""

import time
import random
from core.decision_system import CentralDecisionSystem
from models import SensorData

def generate_random_sensor_data() -> SensorData:
    """Generate random sensor data for testing."""
    return SensorData(
        temperature=random.uniform(15, 30),
        humidity=random.uniform(40, 70),
        soil_moisture=random.uniform(20, 60),
        wind_speed=random.uniform(0, 10),
        terrain_type=random.choice(["sand", "clay", "loam"]),
        damage_areas=[(random.uniform(50, 55), random.uniform(10, 15))],
        injection_depth=random.uniform(5, 20),
        bubble_flow=random.uniform(10, 100)
    )

if __name__ == "__main__":
    decision_system = CentralDecisionSystem()
    print("Test sensor generator started. Press Ctrl+C to stop.")
    try:
        while True:
            sensor_data = generate_random_sensor_data()
            decision_system.update_sensor_data(sensor_data)
            print("Generated sensor data:")
            for field in sensor_data.__dataclass_fields__:
                print(f"  {field}: {getattr(sensor_data, field)}")
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nTest sensor generator stopped.")
