# -*- coding: utf-8 -*-

from biscuit.key import Key
from typing import List
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

	def has_key(self, key: Key) -> bool:
		"""
		Check if the key exists in the database.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def import_key(self, key: Key) -> bool:
		"""
		Import a key into the database.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def list_keys(self) -> List:
		"""
		List all keys in the database.
		"""
		raise NotImplementedError("Subclasses must implement this method.")
