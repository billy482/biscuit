# -*- coding: utf-8 -*-

from biscuit.config import check_no_check, check_value
import rich.table as table
from typing import Callable, Dict

def check_configuration(config: dict, new_config: Dict) -> None:
	"""
	Check the configuration of the database driver.
	"""
	new_config['path'] = check_value('database.path', config['path'], './biscuit.db', str, check_no_check)

def show_configuration(table: table.Table, add_value: Callable, config: dict) -> None:
	"""
	Show the configuration of the database driver.
	"""
	add_value(table, "database.path", config['path'])
