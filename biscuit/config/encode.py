# -*- coding: utf-8 -*-

from typing import Any, Dict, List, Optional

strOpt = Optional[str]

def save_config(config: Dict[str, Any], filename: strOpt = None, format: strOpt = None) -> str:
	"""
	Saves a configuration dictionary to a file in the specified format (JSON, TOML, or YAML).
	The function recursively parses the input configuration, converting nested dictionaries and lists,
	and extracting values from objects using their `get()` method. The resulting configuration is then
	saved to a file, with the filename and format determined by the provided arguments.

	Args:
		config (Dict[str, Any]): The configuration dictionary to save. Nested dictionaries, lists, and objects
			with a `get()` method are supported.
		filename (Optional[str]): The name of the file to save the configuration to. If not provided, a default
			filename is generated based on the specified format.
		format (Optional[str]): The format to save the configuration in. Supported values are 'json', 'toml', and 'yaml'.
			If not provided, the format is inferred from the filename extension.
		str: The name of the file the configuration was saved to.

	Raises:
		ValueError: If neither 'filename' nor 'format' is specified, or if the format/extension is not supported.
	"""

	def _parse_config_dict(config: Dict[str, Any]) -> Dict[str, Any]:
		"""
		Recursively parses a configuration dictionary, converting nested dictionaries and lists
		by calling appropriate parsing functions, and extracting values from other objects using their `get()` method.

		Args:
			config (Dict[str, Any]): The configuration dictionary to parse.

		Returns:
			Dict[str, Any]: A new dictionary with all nested dictionaries and lists parsed,
			and other values extracted using their `get()` method.
		"""
		new_config = {}
		for key, value in config.items():
			if isinstance(value, Dict):
				new_config[key] = _parse_config_dict(value)
			elif isinstance(value, List):
				new_config[key] = _parse_config_list(value)
			else:
				new_config[key] = value.get()
		return new_config

	def _parse_config_list(config: List[Any]) -> List[Any]:
		"""
		Recursively parses a list of configuration values, handling nested dictionaries and lists.

		Each element in the input list is processed as follows:
		- If the element is a dictionary, it is parsed using the `_parse_config_dict` function.
		- If the element is a list, it is recursively parsed using `_parse_config_list`.
		- Otherwise, it is assumed to be an object with a `get()` method, and the result of `val.get()` is appended.

		Args:
			config (List[Any]): The list of configuration values to parse.

		Returns:
			List[Any]: A new list with all nested dictionaries and lists parsed, and other values replaced by the result of their `get()` method.
		"""
		new_config = []
		for val in config:
			if isinstance(val, Dict):
				new_config.append(_parse_config_dict(val))
			elif isinstance(val, List):
				new_config.append(_parse_config_list(val))
			else:
				new_config.append(val.get())
		return new_config
	
	if filename is None:
		if format is None:
			raise ValueError("Either 'filename' or 'format' must be specified.")
		elif format in ['json', 'toml', 'yaml']:
			filename = 'biscuit.' + format
		else:
			raise ValueError("If 'format' is specified, 'filename' must also be provided.")

	new_config = _parse_config_dict(config)

	if format is not None:
		if format == 'json':
			_save_config_json(new_config, filename)
		elif format == 'toml':
			_save_config_toml(new_config, filename)
		elif format == 'yaml':
			_save_config_yaml(new_config, filename)
	elif filename.endswith('.json'):
		_save_config_json(new_config, filename)
	elif filename.endswith('.toml'):
		_save_config_toml(new_config, filename)
	elif filename.endswith('.yaml'):
		_save_config_yaml(new_config, filename)
	else:
		raise ValueError(f"File extension not supported for this file ({filename}). Supported extensions are .json, .toml, and .yaml")

	return filename

def _save_config_json(config: Dict[str, Any], filename: str) -> None:
	"""
	Save a configuration dictionary to a JSON file.

	Args:
		config (Dict[str, Any]): The configuration data to save.
		filename (str): The path to the file where the JSON data will be written.

	Returns:
		None

	The function writes the given configuration dictionary to the specified file in JSON format,
	using an indentation of 4 spaces and preserving non-ASCII characters.
	"""
	import json

	with open(filename, 'w') as fd:
		json.dump(config, fd, indent=4, ensure_ascii=False)

def _save_config_toml(config: Dict[str, Any], filename: str) -> None:
	"""
	Save a configuration dictionary to a TOML file.

	Args:
		config (Dict[str, Any]): The configuration data to save.
		filename (str): The path to the TOML file where the configuration will be saved.

	Raises:
		OSError: If the file cannot be opened or written to.
		toml.TomlEncodeError: If the configuration cannot be serialized to TOML.
	"""
	import toml

	with open(filename, 'w') as fd:
		toml.dump(config, fd)

def _save_config_yaml(config: Dict[str, Any], filename: str) -> None:
	"""
	Save a configuration dictionary to a YAML file.

	Args:
		config (Dict[str, Any]): The configuration data to be saved.
		filename (str): The path to the YAML file where the configuration will be written.

	Returns:
		None

	Raises:
		OSError: If the file cannot be opened or written to.
	"""
	import yaml

	with open(filename, 'w') as fd:
		yaml.dump(config, fd, default_flow_style=False, allow_unicode=True, sort_keys=False)
