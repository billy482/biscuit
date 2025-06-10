# -*- coding: utf-8 -*-

import argparse
from typing import Dict

def _backup(args: argparse.Namespace, config: Dict) -> int:
	from biscuit.database import Driver
	from biscuit.io import parse_config as parse_path_config
	from biscuit.key import Key
	from concurrent.futures import ThreadPoolExecutor
	from hashlib import sha256
	import json
	import logging
	from os import cpu_count
	from threading import Lock

	logger = logging.getLogger('biscuit.core')
	logger.info("Starting backup process...")

	nb_workers = cpu_count() or 1
	tp_database = ThreadPoolExecutor(max_workers = 1, thread_name_prefix = 'database_worker_')
	tp_workers = ThreadPoolExecutor(max_workers = nb_workers, thread_name_prefix = 'workers_')

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

	backup_id = connection.start_backup()
	if backup_id is None:
		logger.error("Failed to start backup.")
		return 1

	sources = parse_path_config(config['backup'])
	def gen_files():
		for source in sources:
			for file in source:
				yield (source, file)

	files = gen_files()
	lock_files = Lock()

	statuses = [ f"Worker #{i}: Waiting for files..." for i in range(nb_workers) ]
	lock_status = Lock()

	def worker(i_worker):
		key = Key(config)
		key.load_public_key()
		key_id = tp_database.submit(connection.get_key, key).result()

		try:
			while True:
				with lock_files:
					source, file = next(files)

				with lock_status:
					statuses[i_worker] = f"Worker #{i_worker}: Processing file {file.path()}"
				logger.info(f"Worker #{i_worker}: Processing file {file.path()} from source {source}")

				host = source.get_host()
				host_id = tp_database.submit(connection.synchronize_host, host).result()

				if tp_database.submit(connection.is_newer_or_not_exists, file, host_id).result():
					file_id = tp_database.submit(connection.get_file, file, host_id).result()

					if file_id is None:
						new_file = True
						file_id = tp_database.submit(connection.insert_file, file, host_id).result()
						logger.debug(f"Worker #{i_worker}: Inserted file {file} with ID {file_id}")
					else:
						new_file = False
						logger.debug(f"Worker #{i_worker}: File {file} already exists with ID {file_id}")

					if file.is_link():
						link = file.read_link().encode()
						link_digest = sha256(link).digest()
						link_id = connection.get_block(link_digest, 'sha256', key_id)
						if link_id is None:
							link_encrypted = key.encrypt(link)
							link_id = connection.insert_block(link_encrypted, link_digest, 'sha256', key_id)
						connection.link_file_to_block(file_id, link_id, 0)
					elif file.is_file():
						from time import time

						sequence = 0
						reader = file.open_for_read()
						last_update = int(time())
						total_read = 0

						while (block_data := reader.read(4096)):
							total_read += len(block_data)

							block_digest = sha256(block_data).digest()
							block_id = tp_database.submit(connection.get_block, block_digest, 'sha256', key_id).result()
							if block_id is None:
								if not new_file:
									file_id = tp_database.submit(connection.modify_file, file_id, file, sequence, host_id).result()
									new_file = True
									logger.debug(f"Worker #{i_worker}: Modified file {file} with new ID {file_id}")

								block_encrypted = key.encrypt(block_data)
								block_id = tp_database.submit(connection.insert_block, block_encrypted, block_digest, 'sha256', key_id).result()
								logger.debug(f"Worker #{i_worker}: Insert new block {block_id}")
							else:
								logger.debug(f"Worker #{i_worker}: Reuse old block {block_id}")

							tp_database.submit(connection.link_file_to_block, file_id, block_id, sequence).result()
							sequence += 1

							current_time = int(time())
							if last_update != current_time:
								last_update = current_time
								pct = 100 * total_read / file.size()
								with lock_status:
									statuses[i_worker] = f"Worker #{i_worker}: Processing file {file.path()}: {total_read} / {file.size()} = {pct:.3f} %"

						reader.close()
				else:
					file_id = tp_database.submit(connection.get_file, file, host_id).result()
					logger.info(f"Worker #{i_worker}: Skipping {file} as it is not newer or does not exist in the database.")

				metadata = json.dumps(file.metadata(), sort_keys = True).encode('utf-8')
				metadata_digest = sha256(metadata).digest()
				metadata_id = tp_database.submit(connection.get_metadata, metadata_digest, 'sha256', key_id).result()
				if metadata_id is None:
					metadata_encrypted = key.encrypt(metadata)
					metadata_id = tp_database.submit(connection.insert_metadata, metadata_encrypted, metadata_digest, 'sha256', key_id).result()
					logger.debug(f"Worker #{i_worker}: Insert new metadata {metadata_id}")
				else:
					logger.debug(f"Worker #{i_worker}: Reuse old metadata {metadata_id}")

				tp_database.submit(connection.link_file_to_backup, file_id, backup_id, metadata_id).result()

		except StopIteration:
			logger.info(f"Worker #{i_worker}: Finished processing files.")

	futures = [ tp_workers.submit(worker, i) for i in range(nb_workers) ]

	if args.progress:
		from time import sleep
		displayed = False
		while not all(f.done() for f in futures):
			if displayed:
				print(f'\x1b[0G\x1b[{nb_workers}A\x1b[K\x1b[0J', end = '')
			else:
				displayed = True

			with lock_status:
				print(*statuses, sep = '\n')
			sleep(1)

	else:
		tp_workers.shutdown()

	connection.finish_backup(backup_id)
	connection.commit()
	logger.info("Backup completed")

	connection.close()

	return 0

def backup_parse(sub_parser: argparse._SubParsersAction) -> None:
	from .sub_backup import parsers

	parser = sub_parser.add_parser('backup', help = "backup files")
	parser.set_defaults(func = _backup)
	sub_parser = parser.add_subparsers()

	parser.add_argument('-p', '--progress', action='store_true', default=False, help='display the progression of backup')

	for p in parsers:
		p(sub_parser)
