from biscuit.database.driver import check_configuration
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
	check_configuration(new_config['database']['driver'].get(), config['database'], new_config['database'])

	return new_config


def _parse_config_json(filename : str) -> Dict:
	import json

	with open(filename, 'r') as fd:
		config = json.load(fd)

	return config


def _parse_config_toml(filename : str) -> Dict:
	import toml

	with open(filename, 'r') as fd:
		config = toml.load(fd)

	return config
