from biscuit.config.value import Value
from typing import Dict


def check_configuration(config: dict, new_config: Dict) -> None:
	"""
	Check the configuration of the database driver.
	"""
	new_config['path'] = Value(
		config['path'],
		'./biscuit.db'
	)


def show_configuration(table, add_value, config: dict) -> None:
	"""
	Show the configuration of the database driver.
	"""
	add_value(table, "database.path", config['path'])
