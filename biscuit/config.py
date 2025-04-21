def parse_config(filename='biscuit.toml'):
	if filename.endswith('.json'):
		return _parse_config_json(filename)
	elif filename.endswith('.toml'):
		return _parse_config_toml(filename)
	else:
		raise ValueError(f"File extension not supported for this file ({filename}). Supported extensions are .json and .toml")


def _parse_config_json(filename):
	import json

	with open(filename, 'r') as fd:
		config = json.load(fd)

	return config


def _parse_config_toml(filename):
	import toml

	with open(filename, 'r') as fd:
		config = toml.load(fd)

	return config
