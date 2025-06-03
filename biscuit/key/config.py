# -*- coding: utf-8 -*-

from biscuit.config.value import Value
from typing import Dict

algos = [
	'AES128-CCM',
	'AES256-CCM',
	'AES128-GCM',
	'AES192-GCM',
	'AES256-GCM',
	'ChaCha20Poly1305',
]

def check_configuration(config: Dict, new_config: Dict) -> None:
	"""
	Validates and updates the provided configuration dictionary for cryptographic settings.

	This function checks if the 'cipher' key in the `config` dictionary is present and valid
	according to the supported algorithms (`algos`). If the specified cipher is invalid or missing,
	it logs a warning and sets the cipher to the default value 'ChaCha20Poly1305' in the `new_config`
	dictionary. It also ensures that the 'path' key is set in `new_config`, defaulting to '~/.biscuit/key'
	if not provided.

	Args:
		config (Dict): The original configuration dictionary to validate.
		new_config (Dict): The dictionary to update with validated configuration values.

	Returns:
		None
	"""
	import logging

	logger = logging.getLogger('biscuit.core')

	if 'cipher' in config:
		if config['cipher'] not in algos:
			logger.warning(
				f"Invalid cipher algorithm '{config['cipher']}' specified. "
				f"Using default 'AES256-GCM'."
			)
			new_config['cipher'] = Value(None, 'AES256-GCM')
		else:
			new_config['cipher'] = Value(config['cipher'], 'AES256-GCM')
	else:
		new_config['cipher'] = Value(None, 'AES256-GCM')

	new_config['path'] = Value(config['path'] if 'path' in config else None, '~/.biscuit/key')
