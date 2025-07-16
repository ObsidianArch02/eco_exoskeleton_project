import logging
import os
from typing import Optional, List

def setup_logging(log_file: Optional[str] = None, level: int = logging.INFO):
    """
    初始化日志系统，支持控制台和可选文件输出。
    :param log_file: 日志文件路径，若为 None 则只输出到控制台。
    :param level: 日志级别。
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