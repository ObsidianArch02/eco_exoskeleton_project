import sys
from eco_exoskeleton.system_controller import EcologicalExoskeletonSystem
from eco_exoskeleton.cli import SystemCLI
from eco_exoskeleton.log_manager import setup_logging
import logging

def main():
    """主程序入口点"""
    setup_logging(log_file="logs/system.log", level=logging.INFO)
    test_mode = "--test" in sys.argv
    exoskeleton_system = EcologicalExoskeletonSystem(test_mode=test_mode)
    cli = SystemCLI(exoskeleton_system)
    cli.start()

if __name__ == "__main__":
    main()
