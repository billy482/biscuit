# -*- coding: utf-8 -*-

import argparse
from biscuit.database import Driver
from biscuit.path import parse_config as parse_path_config
import logging
from typing import Dict


def _backup(args: argparse.Namespace, config: Dict) -> int:
	logger = logging.getLogger('biscuit.core')
	logger.info("Starting backup process...")

	sources = parse_path_config(config['backup'])
	for source in sources:
		for file in source:
			print(file)

	driver = Driver.get_driver(config['database'])
	connection = driver.connect()
	if connection is None:
		logger.error("Failed to connect to the database.")
		return 1
	else:
		logger.info("Connected to the database.")

	return 0


def backup_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('backup', help="backup files")
	parser.set_defaults(func=_backup)
