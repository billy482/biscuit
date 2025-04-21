import argparse


def config_parse(sub_parser: argparse._SubParsersAction):
	from .sub_config import parsers

	parser = sub_parser.add_parser('config', help="Show configuration")
	sub_parser = parser.add_subparsers(dest="Command", help="Subcommand help")

	for p in parsers:
		p(sub_parser)
