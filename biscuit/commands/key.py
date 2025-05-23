# -*- coding: utf-8 -*-

import argparse


def key_parse(sub_parser: argparse._SubParsersAction) -> None:
	from .sub_key import parsers

	parser = sub_parser.add_parser('key', help="Manage keys")
	sub_parser = parser.add_subparsers(dest="Command", help="Subcommand help")

	for p in parsers:
		p(sub_parser)
