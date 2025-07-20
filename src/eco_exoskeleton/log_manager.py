import logging
import os
from typing import Optional, List

def setup_logging(log_file: Optional[str] = None, level: int = logging.INFO):
    """
    Initialize the logging system, supporting both console and optional file output.
    :param log_file: Path to the log file. If None, output is only to the console.
    :param level: Logging level.
    """
    log_format = '[%(asctime)s] %(levelname)s [%(name)s]: %(message)s'
    datefmt = '%Y-%m-%d %H:%M:%S'
    handlers: List[logging.Handler] = [logging.StreamHandler()]
    
    if log_file:
        log_dir = os.path.dirname(log_file)
        if log_dir:
            os.makedirs(log_dir, exist_ok=True)
        handlers.append(logging.FileHandler(log_file, encoding='utf-8'))
    
    logging.basicConfig(
        level=level,
        format=log_format,
        datefmt=datefmt,
        handlers=handlers
    ) 