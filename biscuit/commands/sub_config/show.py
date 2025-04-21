import argparse


def _show(args, config):
	"""Show configuration"""
	import rich.console as console
	import rich.table as table

	config_table = table.Table(title=f"Configuration of {args.config}")
	config_table.add_column("Key", justify="left", style="cyan")
	config_table.add_column("Value", justify="right", style="green")
	config_table.add_column("Default value", justify="right", style="green")

	def add_value(table, key: str, val):
		table.add_row(key, str(val.get_current()), str(val.get_default()))

	add_value(config_table, "common.block_size", config['common']['block_size'])

	terminal = console.Console()
	terminal.print(config_table)

	return 0


def show_parse(sub_parser: argparse._SubParsersAction):
	parser = sub_parser.add_parser('show', help="Show configuration")
	parser.set_defaults(func=_show)
