# -*- coding: utf-8 -*-

from typing import Any, Dict, List, Optional

strOpt = Optional[str]

def save_config(config: Dict[str, Any], filename: strOpt = None, format: strOpt = None) -> None:
	"""
	Save the configuration dictionary to a file in JSON, TOML, or YAML format.

	Args:
		config (Dict[str, Any]): The configuration dictionary to save.
		filename (str): The path to the file where the configuration will be saved. Defaults to 'biscuit.yaml'.

	Raises:
		ValueError: If the file extension is not supported (not JSON, TOML, or YAML).
	"""
	def _parse_config_dict(config: Dict[str, Any]) -> Dict[str, Any]:
		"""
		Recursively parses a configuration dictionary, extracting values from objects with a `get()` method.

		Args:
			config (Dict[str, Any]): The configuration dictionary to parse. Values can be nested dictionaries or objects with a `get()` method.

		Returns:
			Dict[str, Any]: A new dictionary with the same structure as `config`, where all values are either recursively parsed dictionaries or the result of calling `get()` on the original value.
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
