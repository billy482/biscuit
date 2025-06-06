# -*- coding: utf-8 -*-

import argparse
import logging
from typing import Any, Dict, List

def _configure_logging(args: argparse.Namespace, config: Dict[str, Any]) -> None:
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

	if args.print_log:
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
		if args.print_log:
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

	# logger is not configured yet, so we use print for errors
	try:
		from biscuit.config import parse_config
		config = parse_config(args.config)

	except FileNotFoundError as e:
		print(f"Configuration file not found: {e}")
		return 1

	except ValueError as e:
		print(f"Invalid value found in configuration file: {e}")
		return 1

	try:
		_configure_logging(args, config['log'])

		if hasattr(args, 'func'):
			return args.func(args, config)
		else:
			return 1

	except (KeyError, ValueError, FileNotFoundError) as e:
		logger = logging.getLogger('biscuit.core')
		logger.exception("A configuration error occurred: %s", e)
		return 1

	except Exception as e:
		logger = logging.getLogger('biscuit.core')
		logger.exception("An unexpected error occurred: %s", e)
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

	parser = argparse.ArgumentParser(description = 'Tool to backup files via ssh into encrypted database')
	parser.add_argument('-c', '--config', help = 'Specify alternative configuration file', metavar = 'FILENAME')
	parser.add_argument('--print-log', action = 'store_true', default = False, help = 'Print log messages to console')

	sub_parser = parser.add_subparsers()

	for p in parsers:
		p(sub_parser)

	return parser.parse_args(argv)
