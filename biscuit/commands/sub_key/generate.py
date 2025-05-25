# -*- coding: utf-8 -*-

import argparse
from biscuit.database import Driver
from biscuit.key import Key
from getpass import getpass
import logging
from typing import Dict

def _generate(args: argparse.Namespace, config: Dict) -> int:
	"""
	Generate a new key.
	"""
	logger = logging.getLogger('biscuit.core')
	logger.info("Generate new key...")

	passphrase = None
	if not args.no_passphrase:
		passphrase = getpass("Enter passphrase for the key: ")
		passphrase_confirm = getpass("Confirm passphrase: ")
		if passphrase != passphrase_confirm:
			logger.error("Passphrases do not match.")
			return 1

	driver = Driver.get_driver(config['database'])
	if driver is None:
		logger.error("No database driver found")
		return 1

	connection = driver.connect()
	if connection is None:
		logger.error("Failed to connect to the database.")
		return 1

	key = Key.generate_key_pair()

	print(f"Generated key: {key}")

	return 0

def generate_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('generate', aliases=["gen"], help="Generate a new rsa key")
	parser.set_defaults(func=_generate)
	parser.add_argument('-P', '--no-passphrase', action='store_true', help="Do not use a passphrase")
