# -*- coding: utf-8 -*-

from typing import Dict
from .option import Option

class Source:
	def __init__(self, config: Dict, option: Option):
		from .config import load_config

		self.source = config['path'].get()
		self.option = Option(config['options'], option)
		self.files = load_config(config)

	def __iter__(self):
		return self

	def __next__(self):
		return next(self.files)
