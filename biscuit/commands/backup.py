# -*- coding: utf-8 -*-

import argparse
from hmac import digest_size
from biscuit.database import Driver
from biscuit.io import parse_config as parse_path_config
from biscuit.key import Key
from hashlib import sha256
import json
import logging
from typing import Dict

def _backup(args: argparse.Namespace, config: Dict) -> int:
	logger = logging.getLogger('biscuit.core')
	logger.info("Starting backup process...")

	driver = Driver.get_driver(config['database'])
	if driver is None:
		print("Error: No database driver found")
		return 1

	connection = driver.connect()
	if connection is None:
		logger.error("Failed to connect to the database.")
		return 1
	else:
		logger.info("Connected to the database.")

	connection.start_transaction()

	key = Key(config)
	key.load_public_key()
	key_id = connection.get_key(key)

	backup_id = connection.start_backup()
	if backup_id is None:
		logger.error("Failed to start backup.")
		return 1

	sources = parse_path_config(config['backup'])
	for source in sources:
		host = source.get_host()
		host_id = connection.synchronize_host(host)

		for file in source:
			if connection.is_newer_or_not_exists(file, host_id):
				file_id = connection.get_file(file, host_id)

				if file_id is None:
					new_file = True
					file_id = connection.insert_file(file, host_id)
					logger.debug(f"Inserted file {file} with ID {file_id}")
				else:
					new_file = False
					logger.debug(f"File {file} already exists with ID {file_id}")

				if file.is_file():
					sequence = 0
					reader = file.open_for_read()
					while (block_data := reader.read(4096)):
						block_digest = sha256(block_data).digest()
						block_id = connection.get_block(block_digest, 'sha256', key_id)
						if block_id is None:
							if not new_file:
								file_id = connection.modify_file(file_id, file, sequence, host_id)
								new_file = True
								logger.debug(f"Modified file {file} with new ID {file_id}")

							block_encrypted = key.encrypt(block_data)
							block_id = connection.insert_block(block_encrypted, block_digest, 'sha256', key_id)
							logger.debug(f"Insert new block {block_id}")
						else:
							logger.debug(f"Reuse old block {block_id}")

						connection.link_file_to_block(file_id, block_id, sequence)
						sequence += 1

					reader.close()
			else:
				file_id = connection.get_file(file, host_id)
				logger.info(f"Skipping {file} as it is not newer or does not exist in the database.")

			metadata = json.dumps(file.metadata(), sort_keys = True).encode('utf-8')
			metadata_digest = sha256(metadata).digest()
			metadata_id = connection.get_metadata(metadata_digest, 'sha256', key_id)
			if metadata_id is None:
				metadata_encrypted = key.encrypt(metadata)
				metadata_id = connection.insert_metadata(metadata_encrypted, metadata_digest, 'sha256', key_id)
				logger.debug(f"Insert new metadata {metadata_id}")
			else:
				logger.debug(f"Reuse old metadata {metadata_id}")

			connection.link_file_to_backup(file_id, backup_id, metadata_id)

	connection.finish_backup(backup_id)
	connection.commit()
	logger.info("Backup completed")

	connection.close()

	return 0

def backup_parse(sub_parser: argparse._SubParsersAction) -> None:
	from .sub_backup import parsers

	parser = sub_parser.add_parser('backup', help="backup files")
	parser.set_defaults(func=_backup)
	sub_parser = parser.add_subparsers()

	for p in parsers:
		p(sub_parser)
