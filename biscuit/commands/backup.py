# -*- coding: utf-8 -*-

import argparse
from turtle import back
from typing import Callable, Dict

def _backup(args: argparse.Namespace, config: Dict) -> int:
	from biscuit.database import Driver
	from biscuit.io import parse_config as parse_path_config
	from biscuit.key import Key
	from concurrent.futures import ThreadPoolExecutor
	import json
	import logging
	from os import cpu_count
	from threading import Condition, Lock

	logger = logging.getLogger('biscuit.core')
	logger.info("Starting backup process...")

	backup_block_size = config['backup']['block_size'].get()
	backup_checksum_name = config['backup']['checksum'].get()
	backup_checksum = _backup_checksum(backup_checksum_name)

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

	cache = {}
	cache_wait = Condition()

	files = gen_files()
	lock_files = Lock()

	host_cache = {}
	host_lock = Lock()

	statuses = [ f"Worker #{i}: Waiting for files..." for i in range(nb_workers) ]
	lock_status = Lock()

	def worker(i_worker):
		from time import time

		key = Key(config)
		key.load_public_key()
		key_id = tp_database.submit(connection.get_key, key).result()

		try:
			nb_files = 0
			while True:
				with lock_files:
					source, file = next(files)

				try:
					nb_files += 1
					with lock_status:
						statuses[i_worker] = f"Worker #{i_worker}: ~{nb_files}: Processing file {file.path()}"
					logger.info(f"Worker #{i_worker}: Processing file {file.path()} from source {source}")

					host = source.get_host()
					with host_lock:
						if host not in host_cache:
							host_cache[host] = tp_database.submit(connection.synchronize_host, host).result()
					host_id = host_cache[host]

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
							link_digest = backup_checksum(link)

							with cache_wait:
								while link_digest in cache:
									logger.debug(f"Worker #{i_worker}: Found link {link_digest} in cache")
									cache_wait.wait()

								link_id = tp_database.submit(connection.get_block, link_digest, backup_checksum_name, key_id).result()
								if link_id is None:
									cache[link_digest] = (link, i_worker)

							if link_id is None:
								link_encrypted = key.encrypt(link)
								future = tp_database.submit(connection.insert_block, link_encrypted, link_digest, backup_checksum_name, key_id)

								with cache_wait:
									del cache[link_digest]
									cache_wait.notify_all()

								link_id = future.result()

							tp_database.submit(connection.link_file_to_block, file_id, link_id, 0).result()

						elif file.is_file():
							sequence = 0
							reader = file.open_for_read()
							last_update = int(time())
							total_read = 0

							while (block_data := reader.read(4096)):
								total_read += len(block_data)

								block_digest = backup_checksum(block_data)
								block_id = None

								with cache_wait:
									while block_digest in cache:
										logger.debug(f"Worker #{i_worker}: Found block {block_digest} in cache")
										cache_wait.wait()

									block_id = tp_database.submit(connection.get_block, block_digest, backup_checksum_name, key_id).result()
									if block_id is None:
										cache[block_digest] = (block_data, i_worker)

								if block_id is None:
									if not new_file:
										file_id = tp_database.submit(connection.modify_file, file_id, file, sequence, host_id).result()
										new_file = True
										logger.debug(f"Worker #{i_worker}: Modified file {file} with new ID {file_id}")

									block_encrypted = key.encrypt(block_data)
									future = tp_database.submit(connection.insert_block, block_encrypted, block_digest, backup_checksum_name, key_id)

									with cache_wait:
										del cache[block_digest]
										cache_wait.notify_all()

									block_id = future.result()
									logger.debug(f"Worker #{i_worker}: Insert new block {block_id}")
								else:
									logger.debug(f"Worker #{i_worker}: Reuse old block {block_id}")

								future = tp_database.submit(connection.link_file_to_block, file_id, block_id, sequence)
								sequence += 1

								current_time = int(time())
								if last_update != current_time:
									last_update = current_time
									pct = 100 * total_read / file.size()
									with lock_status:
										statuses[i_worker] = f"Worker #{i_worker}: ~{nb_files}: Processing file {file.path()}: {total_read} / {file.size()} = {pct:.3f} %"

								future.result()

							reader.close()
					else:
						file_id = tp_database.submit(connection.get_file, file, host_id).result()
						logger.info(f"Worker #{i_worker}: Skipping {file} as it is not newer or does not exist in the database.")

					# Update file metadata
					metadata = json.dumps(file.metadata(), sort_keys = True).encode('utf-8')
					metadata_digest = backup_checksum(metadata)

					with cache_wait:
						while metadata_digest in cache:
							logger.debug(f"Worker #{i_worker}: Found metadata {metadata_digest} in cache")
							cache_wait.wait()

						metadata_id = tp_database.submit(connection.get_metadata, metadata_digest, backup_checksum_name, key_id).result()
						if metadata_id is None:
							cache[metadata_digest] = (metadata, i_worker)

					if metadata_id is None:
						metadata_encrypted = key.encrypt(metadata)
						future = tp_database.submit(connection.insert_metadata, metadata_encrypted, metadata_digest, backup_checksum_name, key_id)

						with cache_wait:
							del cache[metadata_digest]
							cache_wait.notify_all()

						metadata_id = future.result()
						logger.debug(f"Worker #{i_worker}: Insert new metadata {metadata_id}")
					else:
						logger.debug(f"Worker #{i_worker}: Reuse old metadata {metadata_id}")

					tp_database.submit(connection.link_file_to_backup, file_id, backup_id, metadata_id).result()

				except Exception as e:
					logger.error(f"Worker #{i_worker}: Error processing file {file.path()}: {e}")

		except StopIteration:
			with lock_status:
				statuses[i_worker] = f"Worker #{i_worker}: Finished processing files."
			logger.info(f"Worker #{i_worker}: Finished processing files.")

		except Exception as e:
			logger.error(f"Worker #{i_worker}: Error processing files: {e}")
			with lock_status:
				statuses[i_worker] = f"Worker #{i_worker}: Error processing files: {e}"
			raise

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

def _backup_checksum(checksum: str) -> Callable[[bytes], bytes]:
	import hashlib

	algos = {
		'md5': hashlib.md5,
		'sha1': hashlib.sha1,
		'sha256': hashlib.sha256,
		'sha512': hashlib.sha512
	}

	func = algos.get(checksum, hashlib.sha1)

	return lambda data: func(data).digest()

def backup_parse(sub_parser: argparse._SubParsersAction) -> None:
	from .sub_backup import parsers

	parser = sub_parser.add_parser('backup', help = "backup files")
	parser.set_defaults(func = _backup)
	sub_parser = parser.add_subparsers()

	parser.add_argument('-p', '--progress', action='store_true', default=False, help='display the progression of backup')

	for p in parsers:
		p(sub_parser)
