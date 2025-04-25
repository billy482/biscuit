# -*- coding: utf-8 -*-

from typing import Dict
from ..iterator import Iterator

class FileIterator(Iterator):
	def __init__(self, config: Dict):
		self.path = config['path'].get()
