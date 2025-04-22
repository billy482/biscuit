import rich.table
from typing import Dict


class Driver:
	"""
	Base class for database drivers.
	"""
	def __init__(self):
		pass

	def connect(self):
		"""
		Connect to the database.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def disconnect(self):
		"""
		Disconnect from the database.
		"""
		raise NotImplementedError("Subclasses must implement this method.")


def check_configuration(driver: str, config: Dict, new_config: Dict) -> None:
	"""
	Check the configuration of the database driver.
	"""
	if driver == 'sqlite':
		from .sqlite.driver import check_configuration as check_config
		check_config(config, new_config)


def show_configuration(table: rich.table, add_value, config: Dict) -> None:
	"""
	Show the configuration of the database driver.
	"""
	add_value(table, "database.driver", config['driver'])

	if config['driver'].get() == 'sqlite':
		from .sqlite.driver import show_configuration as show_config
		show_config(table, add_value, config)
