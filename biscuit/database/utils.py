# -*- coding: utf-8 -*-

import rich.table
from typing import Dict


def check_configuration(driver: str, config: Dict, new_config: Dict) -> None:
	"""
	Check the configuration of the database driver.
	"""
	if driver == 'sqlite':
		from .sqlite import check_configuration as check_config
		check_config(config, new_config)


def show_configuration(table: rich.table, add_value, config: Dict) -> None:
	"""
	Show the configuration of the database driver.
	"""
	add_value(table, "database.driver", config['driver'])

	if config['driver'].get() == 'sqlite':
		from .sqlite import show_configuration as show_config
		show_config(table, add_value, config)
