import sys
from core.system_controller import EcologicalExoskeletonSystem
from cli import SystemCLI
from core.log_manager import setup_logging
import logging

if __name__ == "__main__":
    setup_logging(log_file="logs/system.log", level=logging.INFO)
    test_mode = "--test" in sys.argv
    exoskeleton_system = EcologicalExoskeletonSystem(test_mode=test_mode)
    cli = SystemCLI(exoskeleton_system)
    cli.start()
