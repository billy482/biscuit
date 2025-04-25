# -*- coding: utf-8 -*-

from typing import Dict

class Driver:
	"""
	Base class for database drivers.
	"""
	def __init__(self):
		pass

	def connect(self) -> "Connection":
		"""
		Connect to the database.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	@staticmethod
	def get_driver(config: Dict) -> 'Driver':
		if config['driver'].get() == 'sqlite':
			from biscuit.database.sqlite import SQLiteDriver
			return SQLiteDriver(config)
		else:
			return None
