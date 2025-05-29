# -*- coding: utf-8 -*-

import argparse
import logging
from typing import Any, Dict, List

def _configure_logging(config: Dict[str, Any]) -> None:
	"""
	Configures logging for the application based on the provided configuration.

	This function sets up logging for different components of the application
	('core', 'database', 'ssh') with specified log levels and handlers. It uses
	both a stream handler for console output and a file handler for logging to
	a file.

	Args:
		config (Dict): A dictionary containing logging configuration. It should
			include:
			- 'path': A dictionary with a `get()` method that returns the file
			  path for the log file.
			- 'levels': A dictionary where keys are component names ('core',
			  'database', 'ssh') and values are objects with a `get()` method
			  that returns the log level as a string ('critical', 'debug',
			  'error', 'info', 'warning').

	Raises:
		KeyError: If required keys ('path', 'levels') or their subkeys are
				  missing in the configuration dictionary.
	"""
	formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

	ch = logging.StreamHandler()
	ch.setFormatter(formatter)

	fh = logging.FileHandler(config['path'].get())
	fh.setFormatter(formatter)

	levels = {
		'critical': logging.CRITICAL,
		'debug': logging.DEBUG,
		'error': logging.ERROR,
		'info': logging.INFO,
		'warning': logging.WARNING
	}

	for type in ['core', 'database', 'keyring', 'ssh']:
		logger = logging.getLogger('biscuit.' + type)
		logger.setLevel(levels[config['levels'][type].get()])
		logger.addHandler(ch)
		logger.addHandler(fh)

def main(argv: List[str]) -> int:
	"""
	Main entry point for the application.

	Parses command-line arguments, loads configuration, sets up logging, and dispatches
	to the appropriate subcommand function if specified.

	Args:
		argv (List[str]): List of command-line arguments.

	Returns:
		int: Exit code. Returns the result of the subcommand function if present, otherwise 1.
	"""
	args = _parse_args(argv)

	from .config import parse_config
	config = parse_config(args.config)
	_configure_logging(config['log'])

	logger = logging.getLogger('biscuit.core')

	from .key import check_configuration as check_config_key
	if not check_config_key(config['key']):
		logger.error("Invalid key configuration, exiting.")
		return 1

	if hasattr(args, 'func'):
		return args.func(args, config)
	else:
		return 1

def _parse_args(argv: List[str]) -> argparse.Namespace:
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
	parser.add_argument('-c', '--config', help='Specify alternative configuration file', metavar='FILENAME')

	sub_parser = parser.add_subparsers()

	for p in parsers:
		p(sub_parser)

	return parser.parse_args(argv)
