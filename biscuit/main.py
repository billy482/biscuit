import argparse
from typing import List


def main(argv: List[str]) -> int:
	args = __parse_args(argv)

	from .config import parse_config
	config = parse_config(args.config)

	if hasattr(args, 'func'):
		return args.func(args, config)
	else:
		return 1


def __parse_args(argv: List[str]) -> argparse.Namespace:
	"""
	Parse command-line arguments.

	This function sets up an argument parser for the application, including
	a description, a configuration file option, and subcommands. It dynamically
	adds subcommand parsers from the `parsers` module and parses the provided
	arguments.

	Args:
		argv (List[str]): A list of command-line arguments to parse.

	Returns:
		argparse.Namespace: An object containing the parsed arguments.
	"""
	from .commands import parsers

	parser = argparse.ArgumentParser(description='Tool to backup files via ssh into encrypted database')
	parser.add_argument('-c', '--config', default='biscuit.toml', help='Specify alternative configuration file (default: biscuit.toml)', metavar='FILENAME')

	sub_parser = parser.add_subparsers(dest="Command", help="Subcommand help")

	for p in parsers:
		p(sub_parser)

	return parser.parse_args(argv)
