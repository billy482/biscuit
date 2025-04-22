from typing import Dict
from .value import Value


def parse_config(filename: str = 'biscuit.toml') -> Dict:
	"""
	Parse a configuration file in JSON or TOML format.

	Args:
		filename (str): Path to the configuration file. Defaults to 'biscuit.toml'.

	Returns:
		dict: Parsed configuration as a dictionary.

	Raises:
		FileNotFoundError: If the file does not exist.
		PermissionError: If the file is not readable.
		ValueError: If the file extension is not supported.
		json.JSONDecodeError: If the JSON file is malformed.
		toml.TomlDecodeError: If the TOML file is malformed.
	"""
	if filename.endswith('.json'):
		config = _parse_config_json(filename)
	elif filename.endswith('.toml'):
		config = _parse_config_toml(filename)
	else:
		raise ValueError(f"File extension not supported for this file ({filename}). Supported extensions are .json and .toml")

	new_config = {}
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
			'strategy': Value(
				config['backup']['strategy'] if 'strategy' in config['backup'] else None,
				'timestamp'
			)
		}
	else:
		new_config['backup'] = {
			'block_size': Value(None, 4096),
			'checksum': Value(None, 'sha256'),
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
	from biscuit.database.driver import check_configuration as check_database_configuration
	check_database_configuration(new_config['database']['driver'].get(), config['database'], new_config['database'])

	if 'key' in config:
		new_config['key'] = {
			'path': Value(
				config['key']['path'] if 'path' in config['key'] else None,
				'~/.biscuit/key'
			)
		}
	else:
		new_config['key'] = {
			'path': Value(None, '~/.biscuit/key')
		}

	if 'log' in config:
		new_config['log'] = {
			'levels': {},
			'path': Value(
				config['log']['path'] if 'path' in config['log'] else None,
				'~/.biscuit/log'
			)
		}

		if 'levels' in config['log']:
			for level in ['core', 'database', 'ssh']:
				new_config['log']['levels'][level] = Value(
					config['log']['levels'][level] if level in config['log']['levels'] else None,
					'info'
				)
		else:
			new_config['log']['levels'] = {
				'core': Value(None, 'info'),
				'database': Value(None, 'info'),
				'ssh': Value(None, 'info')
			}
	else:
		new_config['log'] = {
			'levels': {
				'core': Value(None, 'info'),
				'database': Value(None, 'info'),
				'ssh': Value(None, 'info')
			},
			'path': Value(None, '~/.biscuit/log')
		}

	return new_config


def _parse_config_json(filename : str) -> Dict:
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


def _parse_config_toml(filename : str) -> Dict:
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
