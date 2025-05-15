# -*- coding: utf-8 -*-

import os
from typing import Dict, List
from ..source import Source

class FileSource(Source):
	def __init__(self, config: Dict):
		self._path = config['path'].get()

	def get_files(self, path: str) -> List[str]:
		return os.listdir(path)

	def is_dir(self, path: str) -> bool:
		return os.path.isdir(path)

	def join(self, path: List[str]) -> str:
		return os.path.join(*path)
