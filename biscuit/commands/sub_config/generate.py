# -*- coding: utf-8 -*-

import argparse
import logging
from typing import Dict

def _generate(args: argparse.Namespace, config: Dict) -> int:
	from biscuit.config import save_config

	logger = logging.getLogger('biscuit.core')
	logger.info(f"Starting generating configuration, path: {args.output}, format: {args.format}...")

	save_config(config, filename=args.output, format=args.format)

	logger.info("Configuration file generated successfully.")

	return 0

def generate_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('generate', aliases = ["gen"], help = "Generate (or convert) a default configuration file")
	parser.set_defaults(func = _generate)
	parser.add_argument('-f', '--format', choices = ['json', 'toml', 'yaml'], default = 'yaml', help = "Format of the configuration file to generate (json, toml, yaml).", metavar = 'FORMAT')
	parser.add_argument('-o', '--output', help = "Output file for the generated configuration. If not specified, defaults to 'biscuit.yaml'.", metavar = 'FILE')
