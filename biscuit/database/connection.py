# -*- coding: utf-8 -*-

from biscuit.key import Key
from typing import Any, List, TypeAlias
from .driver import Driver

BackupId : TypeAlias = Any

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

	def commit(self) -> bool:
		"""
		Commit the current transaction.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def finish_backup(self, backup_id: BackupId) -> bool:
		"""
		Finish a backup process.
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

	def rollback(self) -> bool:
		"""
		Rollback the current transaction.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def start_backup(self) -> BackupId:
		"""
		Start a backup process.
		"""
		raise NotImplementedError("Subclasses must implement this method.")
