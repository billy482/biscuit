# -*- coding: utf-8 -*-

import argparse
from typing import Dict

def _list(args: argparse.Namespace, config: Dict) -> int:
	"""List backup"""
	from biscuit.database import Driver
	import logging
	import rich.console as console
	from rich.filesize import decimal
	import rich.table as table

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

	backups = connection.list_backups()

	backup_table = table.Table(title="Backups")
	backup_table.add_column("ID", justify="left", style="cyan")
	backup_table.add_column("Start time", justify="left", style="green")
	backup_table.add_column("End time", justify="left", style="green")
	backup_table.add_column("Size", justify="left", style="green")
	backup_table.add_column("Increment size", justify="left", style="green")

	def add_backup(table, backup):
		table.add_row(str(backup['id']), str(backup['start_time']), str(backup['end_time']), decimal(backup['size']), decimal(backup['increment_size']))

	for backup in backups:
		add_backup(backup_table, backup)

	terminal = console.Console()
	terminal.print(backup_table)

	return 0

def list_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('list', help = "List backups")
	parser.set_defaults(func = _list)
