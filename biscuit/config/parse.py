# -*- coding: utf-8 -*-

from typing import Any, Callable, Dict, List, Optional, Type
from .value import Value

strOpt = Optional[str]

def check_config_value(config: Dict[str, Any], path: str, default_value: Any, type: Type, check: Callable) -> Value:
	"""
	Checks and retrieves a value from a nested configuration dictionary using a dot-separated path, 
	validates its type, and applies a custom check function.

	Args:
		config (Dict[str, Any]): The configuration dictionary to search.
		path (str): Dot-separated string representing the path to the desired value.
		default_value (Any): The default value to use if the path does not exist in the config.
		type (Type): The expected type of the value.
		check (Callable): A function that takes the value and raises ValueError if the value is invalid.

	Returns:
		Value: An instance of Value containing the found value (or None) and the default value.

	Raises:
		ValueError: If the value exists but is not of the expected type, or if the check function fails.
	"""
	value = None

	for key in path.split('.'):
		if key in config:
			config = config[key]
		else:
			value = None
			break
	else:
		value = config

	if value is None:
		return Value(None, default_value)
	elif not isinstance(value, type):
		raise ValueError(f"Invalid value type: expected {type.__name__}, got {type(value).__name__}")

	try:
		check(value)

	except ValueError as e:
		raise ValueError(f"Invalid value: {value} for {path} does not pass the check. Error: {e}") from e

	return Value(value, default_value)

def check_value(path: str, value: Any, default_value: Any, type: Type, check: Callable) -> Value:
	"""
	Checks and retrieves a value from a nested configuration dictionary using a dot-separated path,
	validates its type, and applies a custom check function.

	Args:
	path (str): Dot-separated string representing the path to the desired value.
	value (Any): The value to check.
	default_value (Any): The default value to use if the path does not exist in the config.
	type (Type): The expected type of the value.
	check (Callable): A function that takes the value and raises ValueError if the value is invalid.

	Returns:
	Value: An instance of Value containing the found value (or None) and the default value.

	Raises:
	ValueError: If the value exists but is not of the expected type, or if the check function fails.
	"""
	if not isinstance(value, type):
		raise ValueError(f"Invalid value type: expected {type.__name__}, got {type(value).__name__}")

	try:
		check(value)

	except ValueError as e:
		raise ValueError(f"Invalid value: {value} for {path} does not pass the check. Error: {e}") from e

	return Value(value, default_value)

def check_is_in_list(valid_values: list) -> Callable[[Any], None]:
	"""
	Returns a function that checks if a value is in a predefined list of valid values.

	Args:
	valid_values (list): A list of valid values.

	Returns:
	Callable[[Any], None]: A function that takes a value and raises ValueError if the value is not in the list.
	"""
	def check(value: Any) -> None:
		if value not in valid_values:
			raise ValueError(f"Invalid value: {value}. Must be one of {', '.join(valid_values)}.")

	return check

def check_no_check(value: Any) -> None:
	"""
	A no-op function that does nothing. Used as a placeholder for checks that are not needed.
	
	Args:
	value (Any): The value to check, which is ignored in this case.
	"""
	pass

def get_config_filename(filename: strOpt = None) -> str:
	"""
	Returns the configuration filename to use.

	If a filename is provided, it is returned as-is. If no filename is provided,
	the function searches for a configuration file named 'biscuit.json', 'biscuit.toml', or 'biscuit.yaml'
	(in that order) in the current directory. The first file found is returned.

	Raises:
		FileNotFoundError: If no configuration file is found and no filename is provided.

	Args:
		filename (Optional[str]): The name of the configuration file to use. If None, the function will search for a default file.

	Returns:
		str: The path to the configuration file.
	"""
	if filename is None:
		import os.path as path

		for ext in ['json', 'toml', 'yaml']:
			if path.exists(f'biscuit.{ext}'):
				return f'biscuit.{ext}'
		else:
			# If no file found, raise an error
			# This is to ensure that the user is aware that they need to provide a configuration file
			# or that they need to generate one using the 'biscuit config generate' command.
			raise FileNotFoundError("No configuration file found. Please provide a valid configuration file.")
	else:
		return filename

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
	import json, toml, yaml

	config = None
	filename = get_config_filename(filename)
	try:
		if filename.endswith('.json'):
			config = _parse_config_json(filename)
		elif filename.endswith('.toml'):
			config = _parse_config_toml(filename)
		elif filename.endswith('.yaml'):
			config = _parse_config_yaml(filename)

	except json.JSONDecodeError as e:
		raise ValueError(f"Invalid JSON configuration file: {filename}. Error: {e}") from e

	except toml.TomlDecodeError as e:
		raise ValueError(f"Invalid TOML configuration file: {filename}. Error: {e}") from e

	except yaml.YAMLError as e:
		raise ValueError(f"Invalid YAML configuration file: {filename}. Error: {e}") from e

	if config is None:
		raise ValueError(f"File extension not supported for this file ({filename}). Supported extensions are .json, .toml, and .yaml")

	def is_valid_power_of_two(n: int):
		if not (128 <= n <= 1024 * 1024 and (n & (n - 1) == 0)):
			raise ValueError(f"Invalid value: {n}. Must be a power of two between 128 and 1,048,576 bytes.")

	new_config: Dict[str, Any] = {}
	# Check backup options
	if 'backup' in config:
		new_config['backup'] = {
			'block_size': check_config_value(config, 'backup.block_size', 4096, int, is_valid_power_of_two),
			'checksum': check_config_value(config, "backup.checksum", 'sha256', str, check_is_in_list(['md5', 'sha1', 'sha256', 'sha512'])),
			'options': {
				'exclude': [],
				'exclude_if_present': [],
				'exclude_other_filesystem': check_config_value(config, "backup.options.exclude_other_filesystem", False, bool, check_no_check)
			},
			'sources': [],
			'strategy': check_config_value(config, 'backup.strategy', 'timestamp', str, check_is_in_list(['timestamp'])
			)
		}

		if 'options' in config['backup']:
			def parse_options(prefix: str, new_config: Dict[str, Any], config: Dict[str, Any]) -> None:
				if 'exclude' in config:
					for exclude in config['exclude']:
						new_config['exclude'].append(check_value('backup.options.exclude[]', exclude, None, str, check_no_check))

				if 'exclude_if_present' in config:
					for exclude in config['exclude_if_present']:
						new_config['exclude_if_present'].append(check_value('backup.options.exclude_if_present[]', exclude, None, str, check_no_check))

				if 'exclude_other_filesystem' in config:
					new_config['exclude_other_filesystem'] = check_value('backup.options.exclude_other_filesystem', config['exclude_other_filesystem'], False, bool, check_no_check)

			parse_options('backup.options', new_config['backup']['options'], config['backup']['options'])

			if 'sources' in config['backup']:
				for source in config['backup']['sources']:
					if 'path' not in source:
						raise ValueError("Missing 'path' in backup source configuration.")

					new_source: Dict[str, Any] = {
						'driver': Value(None, 'file'),
						'path': check_value('backup.sources[].path', source['path'], None, str, check_no_check),
						'options': {
							'exclude': [],
							'exclude_if_present': [],
							'exclude_other_filesystem': Value(None, False)
						}
					}

					if 'driver' in source:
						new_source['driver'] = check_value('backup.sources[].driver', source['driver'], 'file', str, check_is_in_list(['file']))

					if 'options' in source:
						parse_options('backup.sources[]', new_source['options'], source['options'])

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
			'driver': check_config_value(config, 'database.driver', 'sqlite', str, check_is_in_list(['sqlite'])),
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
			'path': check_config_value(config, 'log.path', '~/.biscuit/log', str, check_no_check)
		}

		if 'levels' in config['log']:
			for level in ['core', 'database', 'keyring', 'ssh']:
				new_config['log']['levels'][level] = check_config_value(config, f'log.levels.{level}', 'info', str, check_is_in_list(['critical', 'debug', 'error', 'info', 'warning']))
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
