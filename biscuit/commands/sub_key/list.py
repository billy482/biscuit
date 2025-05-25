# -*- coding: utf-8 -*-

import argparse
from biscuit.database import Driver
from typing import Dict

def _list(args: argparse.Namespace, config: Dict) -> int:
	"""
	List all keys in the database.
	"""
	driver = Driver.get_driver(config['database'])
	if driver is None:
		print("Error: No database driver found")
		return 1

	connection = driver.connect()
	if connection is None:
		return 1

	keys = connection.list_keys()

	print("Keys from database:")
	for key in keys:
		print(f"ID: {key[0]}, Fingerprint: {key[1]}, Length: {key[2]}")

	return 0

def list_parse(sub_parser: argparse._SubParsersAction) -> None:
	"""
	Parse the subcommand for listing keys.
	"""
	parser = sub_parser.add_parser('list', help="List all keys")
	parser.set_defaults(func=_list)
