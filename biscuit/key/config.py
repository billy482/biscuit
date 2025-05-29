# -*- coding: utf-8 -*-

import logging
from typing import Any, Dict

def check_configuration(config: Dict[str, Any]) -> bool:
	"""
	Check if the configuration is valid.
	
	Args:
		config (Dict[str, Any]): The configuration dictionary to check.
		
	Returns:
		bool: True if the configuration is valid, False otherwise.
	"""
	logger = logging.getLogger('biscuit.core')

	algo = config['algo'].get()
	if algo not in ['AES128', 'AES256', 'Camellia', 'ChaCha20']:
		logger.error(f"Unsupported algorithm: {algo}")
		return False

	mode = config['mode'].get()
	padding = config['padding'].get()
	if mode not in ['CBC', 'CFB', 'CFB8', 'CTR', 'GCM', 'OFB']:
		logger.error(f"Unsupported mode: {mode}")
		return False
	elif mode in ['CBC']:
		if padding not in ['ANSIX923', 'PKCS7']:
			logger.error(f"Unsupported padding: {padding}")
			return False
		logger.debug(f"Configuration is valid: {config}")
		logger.debug(f"Algorithm: {algo}, Mode: {mode}, Padding: {padding}")
	else:
		logger.debug(f"Configuration is valid: {config}")
		logger.debug(f"Algorithm: {algo}, Mode: {mode}")
		logger.debug(f"Mode {mode} does not require padding, skipping check.")

	return True
