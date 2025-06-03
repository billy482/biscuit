# -*- coding: utf-8 -*-

import argparse
from typing import Dict

def _show(args: argparse.Namespace, config: Dict) -> int:
	"""Show configuration"""
	from biscuit.database import show_configuration as show_database_configuration
	import rich.console as console
	import rich.table as table

	config_table = table.Table(title=f"Configuration of {args.config}")
	config_table.add_column("Key", justify="left", style="cyan")
	config_table.add_column("Value", justify="right", style="green")
	config_table.add_column("Default value", justify="right", style="green")

	def add_value(table: table.Table, key: str, val):
		table.add_row(key, str(val.get_current()), str(val.get_default()))

	add_value(config_table, "backup.block_size", config['backup']['block_size'])
	add_value(config_table, "backup.checksum", config['backup']['checksum'])
	for i in range(len(config['backup']['options']['exclude'])):
		add_value(config_table, f"backup.options.exclude[{i}]", config['backup']['options']['exclude'][i])
	for i in range(len(config['backup']['options']['exclude_if_present'])):
		add_value(config_table, f"backup.options.exclude_if_present[{i}]", config['backup']['options']['exclude_if_present'][i])
	add_value(config_table, "backup.options.exclude_other_filesystem", config['backup']['options']['exclude_other_filesystem'])
	for i in range(len(config['backup']['sources'])):
		add_value(config_table, f"backup.sources[{i}].driver", config['backup']['sources'][i]['driver'])
		add_value(config_table, f"backup.sources[{i}].path", config['backup']['sources'][i]['path'])
		for j in range(len(config['backup']['sources'][i]['options']['exclude'])):
			add_value(config_table, f"backup.sources[{i}].options.exclude[{j}]", config['backup']['sources'][i]['options']['exclude'][j])
		for j in range(len(config['backup']['sources'][i]['options']['exclude_if_present'])):
			add_value(config_table, f"backup.sources[{i}].options.exclude_if_present[{j}]", config['backup']['sources'][i]['options']['exclude_if_present'][j])
		add_value(config_table, f"backup.sources[{i}].options.exclude_other_filesystem", config['backup']['sources'][i]['options']['exclude_other_filesystem'])
	add_value(config_table, "backup.strategy", config['backup']['strategy'])

	show_database_configuration(config_table, add_value, config['database'])

	add_value(config_table, "key.cipher", config['key']['cipher'])
	add_value(config_table, "key.path", config['key']['path'])

	add_value(config_table, "log.levels.core", config['log']['levels']['core'])
	add_value(config_table, "log.levels.database", config['log']['levels']['database'])
	add_value(config_table, "log.levels.keyring", config['log']['levels']['keyring'])
	add_value(config_table, "log.levels.ssh", config['log']['levels']['ssh'])
	add_value(config_table, "log.path", config['log']['path'])


	terminal = console.Console()
	terminal.print(config_table)

	return 0

def show_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('show', help = "Show configuration")
	parser.set_defaults(func = _show)
