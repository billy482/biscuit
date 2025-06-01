# -*- coding: utf-8 -*-

from typing import Any, Dict, Optional
from .value import Value

strOpt = Optional[str]

def parse_config(filename: strOpt = None) -> Dict[str, Any]:
	"""
	Parses a configuration file for the Biscuit application, supporting JSON, TOML, and YAML formats.

	If no filename is provided, the function searches for 'biscuit.json', 'biscuit.toml', or 'biscuit.yaml'
	in the current directory, in that order. If none are found, a FileNotFoundError is raised.

	The function loads the configuration, applies default values for missing options, and returns a
	normalized configuration dictionary with all expected fields and defaults.

	Args:
		filename (Optional[str]): The path to the configuration file. If None, the function will
			attempt to find a configuration file in the current directory.

	Returns:
		Dict[str, Any]: A dictionary containing the normalized configuration, with all required
			sections ('backup', 'database', 'key', 'log') and their respective options, each
			wrapped in Value objects where appropriate.

	Raises:
		FileNotFoundError: If no configuration file is found and no filename is provided.
		ValueError: If the file extension is not supported.
	"""
	if filename is None:
		import os.path as path

		for ext in ['json', 'toml', 'yaml']:
			if path.exists(f'biscuit.{ext}'):
				filename = f'biscuit.{ext}'
				break
		else:
			# If no file found, raise an error
			# This is to ensure that the user is aware that they need to provide a configuration file
			# or that they need to generate one using the 'biscuit config generate' command.
			raise FileNotFoundError("No configuration file found. Please provide a valid configuration file.")

	if filename.endswith('.json'):
		config = _parse_config_json(filename)
	elif filename.endswith('.toml'):
		config = _parse_config_toml(filename)
	elif filename.endswith('.yaml'):
		config = _parse_config_yaml(filename)
	else:
		raise ValueError(f"File extension not supported for this file ({filename}). Supported extensions are .json, .toml, and .yaml")

	new_config: Dict[str, Any] = {}
	# Check common options
	if 'backup' in config:
		new_config['backup'] = {
			'block_size': Value(
				config['backup']['block_size'] if 'block_size' in config['backup'] else None,
				4096
			),
			'checksum': Value(
				config['backup']['checksum'] if 'checksum' in config['backup'] else None,
				'sha256'
			),
			'options': {
				'exclude': [],
				'exclude_if_present': [],
				'exclude_other_filesystem': Value(None, False)
			},
			'sources': [],
			'strategy': Value(
				config['backup']['strategy'] if 'strategy' in config['backup'] else None,
				'timestamp'
			)
		}

		if 'options' in config['backup']:
			def parse_options(new_config: Dict[str, Any], config: Dict[str, Any]) -> None:
				if 'exclude' in config:
					for exclude in config['exclude']:
						new_config['exclude'].append(Value(exclude, None))

				if 'exclude_if_present' in config:
					for exclude in config['exclude_if_present']:
						new_config['exclude_if_present'].append(Value(exclude, None))

				if 'exclude_other_filesystem' in config:
					new_config['exclude_other_filesystem'] = Value(
						config['exclude_other_filesystem'],
						False
					)

			parse_options(new_config['backup']['options'], config['backup']['options'])

			if 'sources' in config['backup']:
				for source in config['backup']['sources']:
					new_source: Dict[str, Any] = {
						'driver': Value(
							source['driver'] if 'driver' in source else None, 'file'),
						'path': Value(source['path'], None),
						'options': {
							'exclude': [],
							'exclude_if_present': [],
							'exclude_other_filesystem': Value(None, False)
						}
					}

					if 'options' in source:
						parse_options(new_source['options'], source['options'])

					new_config['backup']['sources'].append(new_source)  # type: ignore
	else:
		new_config['backup'] = {
			'block_size': Value(None, 4096),
			'checksum': Value(None, 'sha256'),
			'options': {
				'exclude': [],
				'exclude_if_present': [],
				'exclude_other_filesystem': Value(None, False)
			},
			'sources': [],
			'strategy': Value(None, 'timestamp')
		}

	if 'database' in config:
		new_config['database'] = {
			'driver': Value(
				config['database']['driver'] if 'driver' in config['database'] else None,
				'sqlite'
			)
		}
	else:
		new_config['database'] = {
			'driver': Value(None, 'sqlite')
		}
	from biscuit.database import check_configuration as check_database_configuration
	check_database_configuration(new_config['database']['driver'].get(), config['database'], new_config['database'])

	new_config['key'] = {}
	from biscuit.key import check_configuration as check_key_configuration
	check_key_configuration(config['key'], new_config['key'])

	if 'log' in config:
		new_config['log'] = {
			'levels': {},
			'path': Value(
				config['log']['path'] if 'path' in config['log'] else None,
				'~/.biscuit/log'
			)
		}

		if 'levels' in config['log']:
			for level in ['core', 'database', 'keyring', 'ssh']:
				new_config['log']['levels'][level] = Value(
					config['log']['levels'][level] if level in config['log']['levels'] else None,
					'info'
				)
		else:
			new_config['log']['levels'] = {
				'core': Value(None, 'info'),
				'database': Value(None, 'info'),
				'keyring': Value(None, 'info'),
				'ssh': Value(None, 'info')
			}
	else:
		new_config['log'] = {
			'levels': {
				'core': Value(None, 'info'),
				'database': Value(None, 'info'),
				'keyring': Value(None, 'info'),
				'ssh': Value(None, 'info')
			},
			'path': Value(None, '~/.biscuit/log')
		}

	return new_config

def _parse_config_json(filename: str) -> Dict[str, Any]:
	"""
	Parse a JSON configuration file and return its contents as a dictionary.

	Args:
		filename (str): The path to the JSON file to be parsed.

	Returns:
		Dict: A dictionary containing the parsed configuration data.

	Raises:
		FileNotFoundError: If the specified file does not exist.
		json.JSONDecodeError: If the file contains invalid JSON.
	"""
	import json

	with open(filename, 'r') as fd:
		config = json.load(fd)

	return config

def _parse_config_toml(filename: str) -> Dict[str, Any]:
	"""
	Parse a TOML configuration file and return its contents as a dictionary.

	Args:
		filename (str): The path to the TOML file to be parsed.

	Returns:
		Dict: A dictionary containing the parsed configuration data.

	Raises:
		FileNotFoundError: If the specified file does not exist.
		toml.TomlDecodeError: If the file is not a valid TOML file.
	"""
	import toml

	with open(filename, 'r') as fd:
		config = toml.load(fd)

	return config

def _parse_config_yaml(filename: str) -> Dict[str, Any]:
	"""
	Parse a YAML configuration file and return its contents as a dictionary.

	Args:
		filename (str): The path to the YAML file to be parsed.

	Returns:
		Dict: A dictionary containing the parsed configuration data.

	Raises:
		FileNotFoundError: If the specified file does not exist.
		yaml.YAMLError: If the file contains invalid YAML.
	"""
	import yaml

	with open(filename, 'r') as fd:
		config = yaml.safe_load(fd)

	return config
