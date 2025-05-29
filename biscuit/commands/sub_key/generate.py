# -*- coding: utf-8 -*-

import argparse
from getpass import getpass
import logging
from typing import Dict

def _generate(args: argparse.Namespace, config: Dict) -> int:
	"""
	Generate a new key.
	"""

	from biscuit.database import Driver
	from biscuit.key import Key

	logger = logging.getLogger('biscuit.core')
	logger.info("Generate new key...")

	passphrase = None
	if not args.no_passphrase:
		passphrase = getpass("Enter passphrase for the key: ")
		passphrase_confirm = getpass("Confirm passphrase: ")
		if passphrase != passphrase_confirm:
			logger.error("Passphrases do not match.")
			return 1

	key = Key.generate_key_pair(config, args.path, args.key_length, passphrase)
	key.save_private_key()
	key.save_public_key()

	logger.info(f"Key generated successfully. Public key fingerprint: {key.fingerprint()}")

	driver = Driver.get_driver(config['database'])
	if driver is None:
		logger.error("No database driver found")
		return 1

	connection = driver.connect()
	if connection is None:
		logger.error("Failed to connect to the database.")
		return 1

	if connection.has_key(key):
		logger.error("Key already exists in the database.")
		return 1

	if not connection.import_key(key):
		logger.error("Failed to import key into the database.")
		return 1
	else:
		logger.info("Key imported into the database successfully.")

	connection.close()

	return 0

def generate_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('generate', aliases=["gen"], help="Generate a new rsa key")
	parser.set_defaults(func=_generate)
	parser.add_argument('-k', '--key-length', type=int, default=2048, help="Length of the key to generate (default: 2048 bits)", metavar='INT')
	parser.add_argument('-p', '--path', default='~/.biscuit/key', help="Path (private key) to the generated key. If not specified, defaults to '~/.biscuit/key'.", metavar='FILE')
	parser.add_argument('-P', '--no-passphrase', action='store_true', help="Do not use a passphrase")
