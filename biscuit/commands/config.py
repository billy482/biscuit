# -*- coding: utf-8 -*-

import argparse

def config_parse(sub_parser: argparse._SubParsersAction) -> None:
	from .sub_config import parsers

	parser = sub_parser.add_parser('config', help = "Show configuration")
	sub_parser = parser.add_subparsers()

	for p in parsers:
		p(sub_parser)
