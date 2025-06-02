# -*- coding: utf-8 -*-

from biscuit import Host
from biscuit.key import Key
from biscuit.io import FileInfo
from typing import Any, List, TypeAlias
from .driver import Driver

BackupId : TypeAlias = Any
BlockId: TypeAlias = Any
FileId : TypeAlias = Any
HostId : TypeAlias = Any
KeyId : TypeAlias = Any
MetadataId : TypeAlias = Any

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

	def get_block(self, hash: bytes, hash_algo: str, key_id: KeyId) -> BlockId:
		"""
		Retrieve a block from the database using its hash, hash algorithm, and key identifier.

		Args:
			hash (bytes): The hash of the block to retrieve.
			hash_algo (str): The name of the hash algorithm used.
			key_id (KeyId): The identifier of the key associated with the block.

		Returns:
			BlockId: The identifier of the retrieved block.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def get_driver(self) -> Driver:
		"""
		Get the driver associated with this connection.
		"""
		return self._driver

	def get_file(self, file_info: FileInfo, host_id: HostId) -> FileId:
		"""
		Get a file from the database.

		Args:
			file_info (FileInfo): The file information to retrieve.
			host_id (HostId): The identifier of the host associated with the file.

		Returns:
			FileId: The identifier of the file.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def get_key(self, key: Key) -> KeyId:
		"""
		Get a key from the database.

		Args:
			key (Key): The key to retrieve.

		Returns:
			KeyId: The identifier of the key.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def get_metadata(self, hash: bytes, hash_algo: str, key_id: KeyId) -> MetadataId:
		"""
		Retrieve metadata associated with a block using its hash and hash algorithm.

		Args:
			hash (bytes): The hash of the block.
			hash_algo (str): The name of the hash algorithm used.
			key_id (KeyId): The identifier of the key associated with the metadata.

		Returns:
			MetadataId: The identifier of the metadata.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

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

	def insert_block(self, block: bytes, hash: bytes, hash_algo: str, key_id: KeyId) -> BlockId:
		"""
		Inserts a block of data into the database.

		Args:
			block (bytes): The block of data to insert.
			hash (bytes): The hash of the block.
			hash_algo (str): The name of the hash algorithm to use.
			key_id (KeyId): The identifier of the key associated with the block.

		Returns:
			BlockId: The identifier of the inserted block.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def insert_file(self, file_info: FileInfo, host_id: HostId) -> FileId:
		"""
		Inserts a file record into the database.

		Args:
			file_info (FileInfo): An object containing information about the file to be inserted.
			host_id (HostId): The identifier of the host associated with the file.

		Returns:
			FileId: The unique identifier of the newly inserted file.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def insert_metadata(self, data: bytes, hash: bytes, hash_algo: str, key_id: KeyId) -> MetadataId:
		"""
		Inserts metadata into the database.

		Args:
			data (bytes): The metadata to insert.
			hash (bytes): The hash of the metadata.
			hash_algo (str): The name of the hash algorithm used.
			key_id (KeyId): The identifier of the key associated with the metadata.

		Returns:
			MetadataId: The identifier of the inserted metadata.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def is_newer_or_not_exists(self, file_info: FileInfo, host_id: HostId) -> bool:
		"""
		Check if the file is newer than the last known version or if it doesn't exist in the database.

		Args:
			file_info (FileInfo): The file information.
			host_id (HostId): The host ID.

		Returns:
			bool: True if the file is newer or doesn't exist, False otherwise.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def link_file_to_backup(self, file_id: FileId, backup_id: BackupId, metadata_id: MetadataId) -> bool:
		"""
		Link a file to a backup in the database.

		Args:
			file_id (FileId): The identifier of the file.
			backup_id (BackupId): The identifier of the backup.
			metadata_id (MetadataId): The identifier of the metadata associated with the file.

		Returns:
			bool: True if the link was successful, False otherwise.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def link_file_to_block(self, file_id: FileId, block_id: BlockId, sequence: int) -> bool:
		"""
		Link a file to a block in the database.

		Args:
			file_id (FileId): The identifier of the file.
			block_id (BlockId): The identifier of the block.
			sequence (int): The sequence number for the file.

		Returns:
			bool: True if the link was successful, False otherwise.
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

	def start_transaction(self) -> bool:
		"""
		Start a new transaction.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def synchronize_host(self, host: Host) -> HostId:
		"""
		Synchronize the host with the database.

		Args:
			host (Host): The host to synchronize.

		Returns:
			HostId: The ID of the synchronized host.
		"""
		raise NotImplementedError("Subclasses must implement this method.")
