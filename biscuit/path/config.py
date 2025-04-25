# -*- coding: utf-8 -*-

from typing import Dict

def load_config(config: Dict):
	from .file import FileIterator

	if 'driver' in config:
		driver = config['driver'].get()
		if driver == 'file':
			return FileIterator(config)
		else:
			raise ValueError(f"Unknown driver: {driver}")
	else:
		return FileIterator(config)

def parse_config(config: Dict):
	from .option import Option
	from .source import Source

	option = Option(config['options'])

	sources = []
	for source in config['sources']:
		sources.append(Source(source, option))

	return sources
