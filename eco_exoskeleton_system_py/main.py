import sys
from core.system_controller import EcologicalExoskeletonSystem
from cli import SystemCLI

if __name__ == "__main__":
    test_mode = "--test" in sys.argv
    exoskeleton_system = EcologicalExoskeletonSystem(test_mode=test_mode)
    cli = SystemCLI(exoskeleton_system)
    cli.start()
