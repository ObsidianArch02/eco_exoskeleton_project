"""
数据库管理模块

支持SQLite持久化存储传感器数据和算法结果。
提供数据查询、统计、清理等功能，解决内存缓存易失问题。
"""

import sqlite3
import json
import time
import threading
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import asdict
from pathlib import Path

logger = logging.getLogger(__name__)

class DatabaseManager:
    """数据库管理器"""
    
    def __init__(self, db_path: str = "sensor_data.db"):
        self.db_path = db_path
        self.lock = threading.Lock()
        self._ensure_directory_exists()
        self._init_database()
        
        logger.info(f"数据库管理器初始化完成: {db_path}")
    
    def _ensure_directory_exists(self):
        """确保数据库目录存在"""
        db_dir = Path(self.db_path).parent
        db_dir.mkdir(parents=True, exist_ok=True)
    
    def _init_database(self):
        """初始化数据库表结构"""
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                # 创建传感器数据表
                conn.execute('''
                    CREATE TABLE IF NOT EXISTS sensor_data (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp REAL NOT NULL,
                        module TEXT NOT NULL,
                        data_type TEXT NOT NULL,
                        value REAL,
                        raw_data TEXT,
                        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                    )
                ''')
                
                # 创建算法结果表
                conn.execute('''
                    CREATE TABLE IF NOT EXISTS algorithm_results (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp REAL NOT NULL,
                        algorithm_name TEXT NOT NULL,
                        module TEXT NOT NULL,
                        data_field TEXT NOT NULL,
                        original_value REAL NOT NULL,
                        processed_value REAL NOT NULL,
                        confidence REAL NOT NULL,
                        metadata TEXT,
                        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                    )
                ''')
                
                # 创建系统状态表
                conn.execute('''
                    CREATE TABLE IF NOT EXISTS system_status (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp REAL NOT NULL,
                        status_type TEXT NOT NULL,
                        module TEXT,
                        status_data TEXT NOT NULL,
                        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                    )
                ''')
                
                # 创建索引
                conn.execute('CREATE INDEX IF NOT EXISTS idx_sensor_timestamp ON sensor_data(timestamp)')
                conn.execute('CREATE INDEX IF NOT EXISTS idx_sensor_module ON sensor_data(module)')
                conn.execute('CREATE INDEX IF NOT EXISTS idx_algorithm_timestamp ON algorithm_results(timestamp)')
                conn.execute('CREATE INDEX IF NOT EXISTS idx_algorithm_name ON algorithm_results(algorithm_name)')
                conn.execute('CREATE INDEX IF NOT EXISTS idx_status_timestamp ON system_status(timestamp)')
                
                conn.commit()
                logger.info("数据库表结构初始化完成")
                
            except Exception as e:
                logger.error(f"数据库初始化失败: {e}")
                conn.rollback()
            finally:
                conn.close()
    
    def store_sensor_data(self, module: str, data: Dict[str, Any], timestamp: Optional[float] = None) -> bool:
        """存储传感器数据"""
        if timestamp is None:
            timestamp = time.time()
        
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                # 存储原始JSON数据
                raw_data_json = json.dumps(data)
                conn.execute('''
                    INSERT INTO sensor_data (timestamp, module, data_type, raw_data)
                    VALUES (?, ?, ?, ?)
                ''', (timestamp, module, 'raw', raw_data_json))
                
                # 分别存储各个数值字段
                for key, value in data.items():
                    if isinstance(value, (int, float)):
                        conn.execute('''
                            INSERT INTO sensor_data (timestamp, module, data_type, value, raw_data)
                            VALUES (?, ?, ?, ?, ?)
                        ''', (timestamp, module, key, float(value), raw_data_json))
                
                conn.commit()
                logger.debug(f"存储传感器数据: {module} - {len(data)} 字段")
                return True
                
            except Exception as e:
                logger.error(f"存储传感器数据失败: {e}")
                conn.rollback()
                return False
            finally:
                conn.close()
    
    def store_algorithm_result(self, timestamp: float, algorithm_name: str, module: str, 
                             data_field: str, original_value: float, processed_value: float,
                             confidence: float, metadata: Dict[str, Any]) -> bool:
        """存储算法处理结果"""
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                metadata_json = json.dumps(metadata)
                conn.execute('''
                    INSERT INTO algorithm_results 
                    (timestamp, algorithm_name, module, data_field, original_value, 
                     processed_value, confidence, metadata)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ''', (timestamp, algorithm_name, module, data_field, original_value,
                      processed_value, confidence, metadata_json))
                
                conn.commit()
                logger.debug(f"存储算法结果: {algorithm_name} - {module}.{data_field}")
                return True
                
            except Exception as e:
                logger.error(f"存储算法结果失败: {e}")
                conn.rollback()
                return False
            finally:
                conn.close()
    
    def store_system_status(self, status_type: str, status_data: Dict[str, Any],
                          module: Optional[str] = None, timestamp: Optional[float] = None) -> bool:
        """存储系统状态"""
        if timestamp is None:
            timestamp = time.time()
        
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                status_json = json.dumps(status_data)
                conn.execute('''
                    INSERT INTO system_status (timestamp, status_type, module, status_data)
                    VALUES (?, ?, ?, ?)
                ''', (timestamp, status_type, module, status_json))
                
                conn.commit()
                logger.debug(f"存储系统状态: {status_type}")
                return True
                
            except Exception as e:
                logger.error(f"存储系统状态失败: {e}")
                conn.rollback()
                return False
            finally:
                conn.close()
    
    def get_sensor_data(self, module: Optional[str] = None, data_type: Optional[str] = None,
                       start_time: Optional[float] = None, end_time: Optional[float] = None,
                       limit: int = 1000) -> List[Dict[str, Any]]:
        """查询传感器数据"""
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            conn.row_factory = sqlite3.Row
            try:
                query = "SELECT * FROM sensor_data WHERE 1=1"
                params = []
                
                if module:
                    query += " AND module = ?"
                    params.append(module)
                
                if data_type:
                    query += " AND data_type = ?"
                    params.append(data_type)
                
                if start_time:
                    query += " AND timestamp >= ?"
                    params.append(start_time)
                
                if end_time:
                    query += " AND timestamp <= ?"
                    params.append(end_time)
                
                query += " ORDER BY timestamp DESC LIMIT ?"
                params.append(limit)
                
                cursor = conn.execute(query, params)
                rows = cursor.fetchall()
                
                result = []
                for row in rows:
                    data = dict(row)
                    if data['raw_data']:
                        try:
                            data['parsed_data'] = json.loads(data['raw_data'])
                        except:
                            data['parsed_data'] = None
                    result.append(data)
                
                return result
                
            except Exception as e:
                logger.error(f"查询传感器数据失败: {e}")
                return []
            finally:
                conn.close()
    
    def get_algorithm_results(self, algorithm_name: Optional[str] = None, module: Optional[str] = None,
                            start_time: Optional[float] = None, end_time: Optional[float] = None,
                            limit: int = 1000) -> List[Dict[str, Any]]:
        """查询算法结果"""
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            conn.row_factory = sqlite3.Row
            try:
                query = "SELECT * FROM algorithm_results WHERE 1=1"
                params = []
                
                if algorithm_name:
                    query += " AND algorithm_name = ?"
                    params.append(algorithm_name)
                
                if module:
                    query += " AND module = ?"
                    params.append(module)
                
                if start_time:
                    query += " AND timestamp >= ?"
                    params.append(start_time)
                
                if end_time:
                    query += " AND timestamp <= ?"
                    params.append(end_time)
                
                query += " ORDER BY timestamp DESC LIMIT ?"
                params.append(limit)
                
                cursor = conn.execute(query, params)
                rows = cursor.fetchall()
                
                result = []
                for row in rows:
                    data = dict(row)
                    if data['metadata']:
                        try:
                            data['parsed_metadata'] = json.loads(data['metadata'])
                        except:
                            data['parsed_metadata'] = None
                    result.append(data)
                
                return result
                
            except Exception as e:
                logger.error(f"查询算法结果失败: {e}")
                return []
            finally:
                conn.close()
    
    def get_statistics(self, hours_back: int = 24) -> Dict[str, Any]:
        """获取统计信息"""
        start_time = time.time() - (hours_back * 3600)
        
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                stats = {}
                
                # 传感器数据统计
                cursor = conn.execute('''
                    SELECT module, data_type, COUNT(*) as count, 
                           AVG(value) as avg_value, MIN(value) as min_value, MAX(value) as max_value
                    FROM sensor_data 
                    WHERE timestamp >= ? AND value IS NOT NULL
                    GROUP BY module, data_type
                ''', (start_time,))
                
                sensor_stats = {}
                for row in cursor.fetchall():
                    module = row[0]
                    if module not in sensor_stats:
                        sensor_stats[module] = {}
                    sensor_stats[module][row[1]] = {
                        'count': row[2],
                        'avg': round(row[3], 2) if row[3] else None,
                        'min': row[4],
                        'max': row[5]
                    }
                stats['sensor_data'] = sensor_stats
                
                # 算法结果统计
                cursor = conn.execute('''
                    SELECT algorithm_name, COUNT(*) as count, AVG(confidence) as avg_confidence
                    FROM algorithm_results 
                    WHERE timestamp >= ?
                    GROUP BY algorithm_name
                ''', (start_time,))
                
                algorithm_stats = {}
                for row in cursor.fetchall():
                    algorithm_stats[row[0]] = {
                        'count': row[1],
                        'avg_confidence': round(row[2], 3) if row[2] else None
                    }
                stats['algorithm_results'] = algorithm_stats
                
                # 总体统计
                cursor = conn.execute('SELECT COUNT(*) FROM sensor_data WHERE timestamp >= ?', (start_time,))
                total_sensor_records = cursor.fetchone()[0]
                
                cursor = conn.execute('SELECT COUNT(*) FROM algorithm_results WHERE timestamp >= ?', (start_time,))
                total_algorithm_records = cursor.fetchone()[0]
                
                stats['summary'] = {
                    'hours_back': hours_back,
                    'total_sensor_records': total_sensor_records,
                    'total_algorithm_records': total_algorithm_records,
                    'start_time': datetime.fromtimestamp(start_time).isoformat()
                }
                
                return stats
                
            except Exception as e:
                logger.error(f"获取统计信息失败: {e}")
                return {}
            finally:
                conn.close()
    
    def cleanup_old_data(self, days_to_keep: int = 30) -> Dict[str, int]:
        """清理旧数据"""
        cutoff_time = time.time() - (days_to_keep * 24 * 3600)
        
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                # 删除旧的传感器数据
                cursor = conn.execute('DELETE FROM sensor_data WHERE timestamp < ?', (cutoff_time,))
                sensor_deleted = cursor.rowcount
                
                # 删除旧的算法结果
                cursor = conn.execute('DELETE FROM algorithm_results WHERE timestamp < ?', (cutoff_time,))
                algorithm_deleted = cursor.rowcount
                
                # 删除旧的系统状态
                cursor = conn.execute('DELETE FROM system_status WHERE timestamp < ?', (cutoff_time,))
                status_deleted = cursor.rowcount
                
                # 优化数据库
                conn.execute('VACUUM')
                
                conn.commit()
                
                result = {
                    'sensor_data_deleted': sensor_deleted,
                    'algorithm_results_deleted': algorithm_deleted,
                    'system_status_deleted': status_deleted,
                    'days_kept': days_to_keep
                }
                
                logger.info(f"数据清理完成: {result}")
                return result
                
            except Exception as e:
                logger.error(f"数据清理失败: {e}")
                conn.rollback()
                return {}
            finally:
                conn.close()
    
    def get_database_info(self) -> Dict[str, Any]:
        """获取数据库信息"""
        with self.lock:
            conn = sqlite3.connect(self.db_path)
            try:
                info = {
                    'database_path': self.db_path,
                    'file_size_mb': Path(self.db_path).stat().st_size / (1024 * 1024) if Path(self.db_path).exists() else 0
                }
                
                # 获取表记录数
                tables = ['sensor_data', 'algorithm_results', 'system_status']
                for table in tables:
                    cursor = conn.execute(f'SELECT COUNT(*) FROM {table}')
                    info[f'{table}_count'] = cursor.fetchone()[0]
                
                # 获取时间范围
                cursor = conn.execute('SELECT MIN(timestamp), MAX(timestamp) FROM sensor_data')
                row = cursor.fetchone()
                if row[0] and row[1]:
                    info['data_time_range'] = {
                        'start': datetime.fromtimestamp(row[0]).isoformat(),
                        'end': datetime.fromtimestamp(row[1]).isoformat()
                    }
                
                return info
                
            except Exception as e:
                logger.error(f"获取数据库信息失败: {e}")
                return {}
            finally:
                conn.close()

# 全局数据库管理器实例
_database_manager = None

def get_database_manager() -> DatabaseManager:
    """获取全局数据库管理器实例"""
    global _database_manager
    if _database_manager is None:
        _database_manager = DatabaseManager()
    return _database_manager 