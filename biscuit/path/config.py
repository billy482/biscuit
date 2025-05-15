# -*- coding: utf-8 -*-

from typing import Dict

def load_config(config: Dict):
	from .file import FileSource

	if 'driver' in config:
		driver = config['driver'].get()
		if driver == 'file':
			return FileSource(config)
		else:
			raise ValueError(f"Unknown driver: {driver}")
	else:
		return FileSource(config)

def parse_config(config: Dict):
	from .file_iterator import FileIterator
	from .filter import Filter

	filter = Filter(config['options'])

	sources = []
	for source in config['sources']:
		sources.append(FileIterator(source, filter))

	return sources
