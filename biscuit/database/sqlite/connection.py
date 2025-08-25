# -*- coding: utf-8 -*-

from biscuit import Host
from biscuit.key import Key
from biscuit.io import FileInfo
import sqlite3
from typing import Any, Dict, List
from .driver import SQLiteDriver
from ..connection import BackupId, BlockId, Connection, FileId, HostId, KeyId, MetadataId

class SQLiteConnection(Connection):
	def __init__(self, connection: sqlite3.Connection, driver: SQLiteDriver):
		import logging

		super().__init__(driver)
		self._connection = connection
		self._logger = logging.getLogger('biscuit.database')

	def close(self) -> bool:
		"""
		Close the connection.
		"""
		self._logger.debug("[SQLite] Closing SQLite connection")

		self._connection.close()
		return True

	def commit(self) -> None:
		"""
		Commit the current transaction.
		"""
		self._logger.debug("[SQLite] Committing transaction")

		try:
			self._connection.commit()

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error committing transaction: {e}")
			raise

	def finish_backup(self, backup_id: BackupId) -> None:
		self._logger.debug(f"[SQLite] finishing backup with ID: {backup_id}")

		cursor = None
		query = "SELECT SUM(LENGTH(data)) FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $1))"
		try:
			cursor = self._connection.execute(query, (backup_id,))
			total_size = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] Total size of backup {backup_id}: {total_size} bytes")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error finishing backup ({backup_id}): {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

		cursor = None
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
			raise

		finally:
			if cursor is not None:
				cursor.close()

		if parent_backup is not None:
			cursor = None
			query = "SELECT COALESCE(SUM(LENGTH(data)), 0) FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $1)) AND id NOT IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $2))"
			try:
				cursor = self._connection.execute(query, (backup_id, parent_backup[0] if parent_backup else None))
				delta_size = cursor.fetchone()[0]
				self._logger.debug(f"[SQLite] Delta size for backup {backup_id}: {delta_size} bytes")

			except sqlite3.Error as e:
				self._logger.error(f"[SQLite] Error calculating delta size for backup ({backup_id}): {e}")
				raise

			finally:
				if cursor is not None:
					cursor.close()

		else:
			delta_size = None

		query = "UPDATE backups SET end_time = unixepoch(), size = $1, increment_size = $2 WHERE id = $3"
		try:
			self._connection.execute(query, (total_size, delta_size, backup_id))
			self._logger.debug(f"[SQLite] Backup {backup_id} finished successfully")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error updating backup ({backup_id}): {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def get_block(self, hash: bytes, hash_algo: str, key_id: KeyId) -> BlockId:
		self._logger.debug(f"[SQLite] Getting block with hash {hash.hex()} using {hash_algo} for key {key_id}")

		cursor = None
		query = "SELECT id FROM blocks WHERE hash = unhex($1) AND hash_algo = $2 AND key = $3 LIMIT 1"
		try:
			cursor = self._connection.execute(query, (hash.hex(), hash_algo, key_id))
			result = cursor.fetchone()
			if result is None:
				self._logger.debug(f"[SQLite] Block with hash {hash.hex()} does not exist")
				return None
			else:
				block_id = result[0]
				self._logger.debug(f"[SQLite] Block with hash {hash.hex()} found with ID {block_id}")
				return block_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching block: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def get_file(self, file_info: FileInfo, host_id: HostId) -> FileId:
		self._logger.debug(f"[SQLite] Getting file at path {file_info.path()} with host ID {host_id}")

		cursor = None
		query = "SELECT id FROM files WHERE path = $1 AND host = $2 LIMIT 1"
		try:
			cursor = self._connection.execute(query, (file_info.path(), host_id))
			result = cursor.fetchone()
			if result is None:
				self._logger.debug(f"[SQLite] File at path {file_info.path()} does not exist")
				return None
			else:
				file_id = result[0]
				self._logger.debug(f"[SQLite] File at path {file_info.path()} found with ID {file_id}")
				return file_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching file: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def get_key(self, key: Key) -> Any:
		self._logger.debug(f"[SQLite] Getting key with fingerprint {key.fingerprint()}")

		cursor = None
		query = "SELECT id FROM keys WHERE fingerprint = $1 LIMIT 1"
		try:
			cursor = self._connection.execute(query, (key.fingerprint(),))
			result = cursor.fetchone()
			if result is None:
				self._logger.debug(f"[SQLite] Key with fingerprint {key.fingerprint()} does not exist")
				return None
			else:
				key_id = result[0]
				self._logger.debug(f"[SQLite] Key with fingerprint {key.fingerprint()} found with ID {key_id}")
				return key_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching key: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def get_metadata(self, hash: bytes, hash_algo: str, key_id: KeyId) -> MetadataId:
		self._logger.debug(f"[SQLite] Getting metadata for hash {hash.hex()} using {hash_algo}")

		cursor = None
		query = "SELECT id FROM metadata WHERE hash = unhex($1) AND hash_algo = $2 AND key = $3 LIMIT 1"
		try:
			cursor = self._connection.execute(query, (hash.hex(), hash_algo, key_id))
			result = cursor.fetchone()
			if result is None:
				self._logger.debug(f"[SQLite] Metadata for hash {hash.hex()} does not exist")
				return None
			else:
				metadata_id = result[0]
				self._logger.debug(f"[SQLite] Metadata for hash {hash.hex()} found with ID {metadata_id}")
				return metadata_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching metadata: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def has_key(self, key: Key) -> bool:
		self._logger.debug(f"[SQLite] Checking if key with fingerprint {key.fingerprint()} exists in the database")

		cursor = None
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
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def import_key(self, key: Key) -> bool:
		self._logger.debug(f"[SQLite] Importing key with fingerprint {key.fingerprint('sha256')}")

		cursor = None
		query = "INSERT INTO keys (fingerprint, hash_algo, length) VALUES (?, ?, ?)"
		params = (key.fingerprint('sha256'), 'sha256', key.key_length())

		try:
			cursor = self._connection.execute(query, params)
			self._logger.debug(f"[SQLite] Key with fingerprint {key.fingerprint('sha256')} imported successfully with ID {cursor.lastrowid}")
			return True

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error importing key ({key.fingerprint('sha256')}): {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def insert_block(self, block: bytes, hash: bytes, hash_algo: str, key_id: KeyId) -> BlockId:
		self._logger.debug(f"[SQLite] Inserting block with hash {key_id} using {hash_algo}")

		cursor = None
		query = "INSERT INTO blocks (hash_algo, hash, data, key) VALUES ($1, unhex($2), unhex($3), $4)"
		try:
			cursor = self._connection.execute(query, (hash_algo, hash.hex(), block.hex(), key_id))
			block_id = cursor.lastrowid
			self._logger.debug(f"[SQLite] Block inserted with ID {block_id}")
			return block_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error inserting block: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def insert_file(self, file_info: FileInfo, host_id: HostId) -> FileId:
		self._logger.debug(f"[SQLite] Inserting file at path {file_info.path()} with host ID {host_id}")

		cursor = None
		query = "INSERT INTO files(path, last_modified, host) VALUES ($1, $2, $3) RETURNING id"
		try:
			cursor = self._connection.execute(query, (file_info.path(), file_info.mtime(), host_id))
			file_id = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] File at path {file_info.path()} inserted with ID {file_id}")
			return file_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error inserting file ({file_info.path()}): {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def insert_metadata(self, data: bytes, hash: bytes, hash_algo: str, key_id: KeyId) -> MetadataId:
		self._logger.debug(f"[SQLite] Inserting metadata with hash {hash.hex()} using {hash_algo} for key ID {key_id}")

		cursor = None
		query = "INSERT INTO metadata(hash_algo, hash, data, key) VALUES ($1, unhex($2), unhex($3), $4) RETURNING id"
		try:
			cursor = self._connection.execute(query, (hash_algo, hash.hex(), data.hex(), key_id))
			metadata_id = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] Metadata inserted with ID {metadata_id}")
			return metadata_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error inserting metadata: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def is_newer_or_not_exists(self, file_info: FileInfo, host_id: HostId) -> bool:
		self._logger.debug(f"[SQLite] Checking if file at path {file_info.path()} is newer or does not exist in the database")

		cursor = None
		query = "SELECT * FROM files WHERE path = $1 AND last_modified >= $2 AND host = $3"
		try:
			cursor = self._connection.execute(query, (file_info.path(), file_info.mtime(), host_id))
			result = cursor.fetchone()

			if result is None:
				self._logger.debug(f"[SQLite] File at path {file_info.path()} does not exist or is newer in the database")
				return True

			else:
				self._logger.debug(f"[SQLite] File at path {file_info.path()} is not newer than the existing one")
				return False

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error checking file existence or modification time: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def link_file_to_backup(self, file_id: FileId, backup_id: BackupId, metadata_id: MetadataId) -> bool:
		self._logger.debug(f"[SQLite] Linking file ID {file_id} to backup ID {backup_id} with metadata ID {metadata_id}")

		query = "INSERT INTO backups2files (backup, file, metadata) VALUES ($1, $2, $3)"
		try:
			self._connection.execute(query, (backup_id, file_id, metadata_id))
			self._logger.debug(f"[SQLite] File ID {file_id} linked to backup ID {backup_id} successfully")
			return True

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error linking file to backup: {e}")
			raise

	def link_file_to_block(self, file_id: Any, block_id: Any, sequence: int) -> bool:
		self._logger.debug(f"[SQLite] Linking file ID {file_id} to block ID {block_id} with sequence {sequence}")

		query = "INSERT INTO files2blocks VALUES ($1, $2, $3)"
		try:
			self._connection.execute(query, (file_id, block_id, sequence))
			self._logger.debug(f"[SQLite] File ID {file_id} linked to block ID {block_id} successfully at sequence {sequence}")
			return True

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error linking file to block: {e}")
			raise

	def list_backups(self) -> List[Dict[str,Any]]:
		from datetime import datetime

		self._logger.info("[SQLite] Listing all backups")

		cursor = None
		query = "SELECT id, start_time, end_time, size, increment_size FROM backups ORDER BY id"

		try:
			cursor = self._connection.cursor()
			cursor.execute(query)

			backups = []
			while row := cursor.fetchone():
				backups.append({
					'id': row[0],
					'start_time': datetime.fromtimestamp(row[1]),
					'end_time': datetime.fromtimestamp(row[2]),
					'size': row[3],
					'increment_size': row[4]
				})

			return backups

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error listing backups: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def list_keys(self) -> List:
		self._logger.info("[SQLite] Listing all keys")

		cursor = None
		query = "SELECT id, fingerprint, length FROM keys ORDER BY id"

		try:
			cursor = self._connection.cursor()
			cursor.execute(query)

			results = cursor.fetchall()
			return results

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error listing keys: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def modify_file(self, old_file_id: FileId, file_info: FileInfo, sequence: int, host_id: HostId) -> FileId:
		self._logger.debug(f"[SQLite] Modifying file with old ID {old_file_id} at sequence {sequence}")

		cursor = None
		query = "INSERT INTO files(path, last_modified, host) VALUES ($1, $2, $3) RETURNING id"
		try:
			cursor = self._connection.execute(query, (file_info.path(), file_info.mtime(), host_id))
			file_id = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] File at path {file_info.path()} inserted with ID {file_id}")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error modifying file: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

		if sequence > 0:
			query = "INSERT INTO files2blocks (file, block, sequence) SELECT $1, block, sequence FROM files2blocks WHERE file = $2 AND sequence < $3 ORDER BY sequence"

			try:
				self._connection.execute(query, (file_id, old_file_id, sequence))
				self._logger.debug(f"[SQLite] Copied blocks from old file ID {old_file_id} to new file ID {file_id} from beginning to sequence {sequence}")

			except sqlite3.Error as e:
				self._logger.error(f"[SQLite] Error updating sequence for file: {e}")
				raise

		return file_id

	def rollback(self) -> None:
		self._logger.debug("[SQLite] Rolling back transaction")
		try:
			self._connection.rollback()

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error rolling back transaction: {e}")
			raise

	def start_backup(self) -> BackupId:
		self._logger.debug("[SQLite] Starting a new backup process")

		cursor = None
		query = "SELECT COALESCE(MAX(id), -1) FROM backups WHERE end_time IS NOT NULL"
		try:
			cursor = self._connection.execute(query)
			backup_seq = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] Last backup ID: {backup_seq}")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error fetching last backup ID: {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

		cursor = None
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
			raise

		finally:
			if cursor is not None:
				cursor.close()

	def start_transaction(self) -> None:
		self._logger.debug("[SQLite] Starting a new transaction")
		try:
			self._connection.execute("BEGIN")
			self._logger.debug("[SQLite] Transaction started successfully")

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error starting transaction: {e}")
			raise

	def synchronize_host(self, host: Host) -> HostId:
		self._logger.debug(f"[SQLite] Synchronizing host: {host.get_host_name()}")

		cursor = None
		query = "SELECT id FROM hosts WHERE hostname = $1 LIMIT 1"
		try:
			cursor = self._connection.execute(query, (host.get_host_name(),))
			result = cursor.fetchone()
			if result is not None:
				host_id = result[0]
				self._logger.debug(f"[SQLite] Host {host.get_host_name()} already exists with ID: {host_id}")
				return host_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error synchronizing host ({host.get_host_name()}): {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()

		cursor = None
		query = "INSERT INTO hosts(hostname) VALUES ($1) RETURNING id"
		try:
			cursor = self._connection.execute(query, (host.get_host_name(),))
			host_id = cursor.fetchone()[0]
			self._logger.debug(f"[SQLite] New host {host.get_host_name()} synchronized with ID: {host_id}")
			return host_id

		except sqlite3.Error as e:
			self._logger.error(f"[SQLite] Error inserting new host ({host.get_host_name()}): {e}")
			raise

		finally:
			if cursor is not None:
				cursor.close()
