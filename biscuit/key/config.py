# -*- coding: utf-8 -*-

from biscuit.config import check_is_in_list, check_no_check, check_value, Value
from typing import Dict

from biscuit.config.parse import check_no_check

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
	if 'cipher' in config:
		new_config['cipher'] = check_value('keyring.cipher', config['cipher'], 'AES256-GCM', str, check_is_in_list(algos))
	else:
		new_config['cipher'] = Value(None, 'AES256-GCM')

	if  'path' in config:
		new_config['path'] = check_value('keyring.path', config['path'], '~/.biscuit/key', str, check_no_check)
	else:
		new_config['path'] = Value(None, '~/.biscuit/key')
