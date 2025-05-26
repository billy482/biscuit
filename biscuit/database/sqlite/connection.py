# -*- coding: utf-8 -*-

from biscuit.key import Key
import logging
import sqlite3
from typing import Any, List
from .driver import SQLiteDriver
from ..connection import BackupId, Connection

class SQLiteConnection(Connection):
	def __init__(self, connection: sqlite3.Connection, driver: SQLiteDriver):
		super().__init__(driver)
		self._connection = connection
		self._logger = logging.getLogger('biscuit.database')

	def close(self) -> bool:
		"""
		Close the connection.
		"""
		logger = logging.getLogger('biscuit.database')
		logger.debug("[SQLite] Closing SQLite connection")

		self._connection.close()
		return True

	def commit(self) -> bool:
		"""
		Commit the current transaction.
		"""
		self._logger.debug("[SQLite] Committing transaction")

		try:
			self._connection.commit()
			return True
		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error committing transaction: {e}")
			return False

	def finish_backup(self, backup_id: BackupId) -> bool:
		self._logger.debug(f"[SQLite] finishing backup with ID: {backup_id}")

		query = "SELECT SUM(LENGTH(data)) FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $1))"
		try:
			cursor = self._connection.execute(query, (backup_id,))
			total_size = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] Total size of backup {backup_id}: {total_size} bytes")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error finishing backup ({backup_id}): {e}")
			return False

		finally:
			cursor.close()

		query = "SELECT parent_backup FROM backups WHERE id = $1 AND parent_backup IS NOT NULL LIMIT 1"
		try:
			cursor = self._connection.execute(query, (backup_id,))
			parent_backup = cursor.fetchone()
			if parent_backup:
				self._logger.debug(f"[SQLite] Parent backup for {backup_id} is {parent_backup[0]}")
			else:
				self._logger.debug(f"[SQLite] No parent backup for {backup_id}")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching parent backup ({backup_id}): {e}")
			return False

		finally:
			cursor.close()

		query = "SELECT COALESCE(SUM(LENGTH(data)), 0) FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $1)) AND id NOT IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $2))"
		try:
			cursor = self._connection.execute(query, (backup_id, parent_backup[0] if parent_backup else None))
			delta_size = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] Delta size for backup {backup_id}: {delta_size} bytes")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error calculating delta size for backup ({backup_id}): {e}")
			return False

		finally:
			cursor.close()

		query = "UPDATE backups SET end_time = unixepoch(), size = $1, increment_size = $2 WHERE id = $3"
		try:
			self._connection.execute(query, (total_size, delta_size, backup_id))
			self._logger.debug(f"[SQLite] Backup {backup_id} finished successfully")
			return True

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error updating backup ({backup_id}): {e}")
			return False

		finally:
			cursor.close()

	def has_key(self, key: Key) -> bool:
		self._logger.debug(f"[SQLite] Checking if key with fingerprint {key.fingerprint()} exists in the database")

		query = "SELECT id FROM keys WHERE fingerprint = $1 LIMIT 1"
		try:
			cursor = self._connection.execute(query, (key.fingerprint(),))
			if cursor.fetchone() is None:
				self._logger.debug(f"[SQLite] Key with fingerprint {key.fingerprint()} does not exist")
				return False
			else:
				self._logger.debug(f"[SQLite] Key with fingerprint {key.fingerprint()} exists")
				return True

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error checking key existence: {e}")
			return False

		finally:
			cursor.close()

	def import_key(self, key: Key) -> bool:
		self._logger.debug(f"[SQLite] Importing key with fingerprint {key.fingerprint('sha256')}")

		query = "INSERT INTO keys (fingerprint, hash_algo, length) VALUES (?, ?, ?)"
		params = (key.fingerprint('sha256'), 'sha256', key.key_length())

		try:
			cursor = self._connection.execute(query, params)
			self._logger.debug(f"[SQLite] Key with fingerprint {key.fingerprint('sha256')} imported successfully with ID {cursor.lastrowid}")
			return True

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error importing key ({key.fingerprint('sha256')}): {e}")
			return False

		finally:
			cursor.close()

	def list_keys(self) -> List:
		query = "SELECT id, fingerprint, length FROM keys ORDER BY id"

		try:
			cursor = self._connection.cursor()
			cursor.execute(query)

			results = cursor.fetchall()
			return results

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error importing key ({key.fingerprint('sha256')}): {e}")
			return []

		finally:
			cursor.close()

	def rollback(self) -> bool:
		self._logger.debug("[SQLite] Rolling back transaction")
		try:
			self._connection.rollback()
			return True
		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error rolling back transaction: {e}")
			return False

	def start_backup(self) -> BackupId:
		self._logger.debug("[SQLite] Starting a new backup process")

		query = "SELECT COALESCE(MAX(id), -1) FROM backups WHERE end_time IS NOT NULL"
		try:
			cursor = self._connection.execute(query)
			backup_seq = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] Last backup ID: {backup_seq}")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching last backup ID: {e}")
			return None

		finally:
			cursor.close()


		query = "INSERT INTO backups (parent_backup) VALUES ($1) RETURNING id"
		try:
			if backup_seq != -1:
				cursor = self._connection.execute(query, (backup_seq, ))
			else:
				cursor = self._connection.execute(query, (None, ))

			backup_id = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] New backup started with ID: {backup_id}")
			return backup_id
		
		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error starting backup: {e}")
			return None
		finally:
			cursor.close()
