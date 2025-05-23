# -*- coding: utf-8 -*-

import sqlite3
from typing import List
from .driver import SQLiteDriver
from ..connection import Connection

class SQLiteConnection(Connection):
	def __init__(self, connection: sqlite3.Connection, driver: SQLiteDriver):
		super().__init__(driver)
		self._connection = connection

	def close(self) -> bool:
		"""
		Close the connection.
		"""
		self._connection.close()
		return True

	def list_keys(self) -> List:
		query = "SELECT id, fingerprint, length FROM keys ORDER BY id"

		cursor = self._connection.cursor()
		cursor.execute(query)

		results = cursor.fetchall()
		cursor.close()

		return results
