# -*- coding: utf-8 -*-

from biscuit.key import Key
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

	def has_key(self, key: Key) -> bool:
		query = "SELECT id FROM keys WHERE fingerprint = ? LIMIT 1"

		cursor = self._connection.cursor()
		cursor.execute(query, (key.fingerprint(),))

		return cursor.rowcount > 0

	def import_key(self, key: Key) -> bool:
		query = "INSERT INTO keys (fingerprint, hash_algo, length) VALUES (?, ?, ?)"
		params = (key.fingerprint('sha256'), 'sha256', key.key_length())

		cursor = self._connection.cursor()
		try:
			cursor.execute(query, params)
			self._connection.commit()
			return True
		except sqlite3.Error as e:
			print(f"Error importing key: {e}")
			self._connection.rollback()
			return False
		finally:
			cursor.close()

	def list_keys(self) -> List:
		query = "SELECT id, fingerprint, length FROM keys ORDER BY id"

		cursor = self._connection.cursor()
		cursor.execute(query)

		results = cursor.fetchall()
		cursor.close()

		return results
