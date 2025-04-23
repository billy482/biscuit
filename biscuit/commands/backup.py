import argparse
from biscuit.database import Driver
import logging
from typing import Dict


def _backup(args: argparse.Namespace, config: Dict) -> int:
	logger = logging.getLogger('biscuit.core')
	logger.info("Starting backup process...")

	driver = Driver.get_driver(config['database'])
	connection = driver.connect()

	return 0


def backup_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('backup', help="backup files")
	parser.set_defaults(func=_backup)
