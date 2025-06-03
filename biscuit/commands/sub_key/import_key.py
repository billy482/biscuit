# -*- coding: utf-8 -*-

import argparse
from typing import Dict

def _import(args: argparse.Namespace, config: Dict) -> int:
	"""
	Imports a public key into the database if it does not already exist.

	Args:
		args (argparse.Namespace): The command-line arguments, expected to contain the file path of the key to import.
		config (Dict): Configuration dictionary containing at least the 'database' key.

	Returns:
		int: 0 if the key was successfully imported or already exists, 1 if an error occurred (e.g., no driver or connection).
	"""
	from biscuit.database import Driver
	from biscuit.key import Key

	driver = Driver.get_driver(config['database'])
	if driver is None:
		print("Error: No database driver found")
		return 1

	connection = driver.connect()
	if connection is None:
		return 1

	key = Key(config, args.file)
	key.load_public_key()

	connection.start_transaction()

	if not connection.has_key(key):
		connection.import_key(key)

	connection.commit()

	return 0

def import_parse(sub_parser: argparse._SubParsersAction) -> None:
	"""
	Parse the subcommand for importing keys.
	"""
	parser = sub_parser.add_parser('import', help = "Import a key into a database from a file")
	parser.add_argument('-f', '--file', help = "Path to the key file to import")
	parser.set_defaults(func = _import)
