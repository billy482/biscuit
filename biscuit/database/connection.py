# -*- coding: utf-8 -*-

from .driver import Driver

class Connection:
	"""
	Base class for database connections.
	"""
	def __init__(self, driver: Driver):
		self._driver = driver

	def close(self) -> bool:
		"""
		Close the connection.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def get_driver(self) -> Driver:
		"""
		Get the driver associated with this connection.
		"""
		return self._driver
