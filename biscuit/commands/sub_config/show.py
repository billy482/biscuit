import argparse
from typing import Dict


def _show(args: argparse.Namespace, config: Dict) -> int:
	"""Show configuration"""
	from biscuit.database.driver import show_configuration as show_database_configuration
	import rich.console as console
	import rich.table as table

	config_table = table.Table(title=f"Configuration of {args.config}")
	config_table.add_column("Key", justify="left", style="cyan")
	config_table.add_column("Value", justify="right", style="green")
	config_table.add_column("Default value", justify="right", style="green")

	def add_value(table, key: str, val):
		table.add_row(key, str(val.get_current()), str(val.get_default()))

	add_value(config_table, "backup.block_size", config['backup']['block_size'])
	add_value(config_table, "backup.checksum", config['backup']['checksum'])
	add_value(config_table, "backup.strategy", config['backup']['strategy'])

	show_database_configuration(config_table, add_value, config['database'])


	terminal = console.Console()
	terminal.print(config_table)

	return 0


def show_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('show', help="Show configuration")
	parser.set_defaults(func=_show)
