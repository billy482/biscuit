# -*- coding: utf-8 -*-

from typing import Dict
from .filter import Filter

class FileIterator:
	def __init__(self, config: Dict, filter: Filter):
		from .config import load_config

		self._path = config['path'].get()
		self._filter = Filter(config['options'], filter)
		self._from = load_config(config)

	def __iter__(self):
		return FileIterator._Iterator(self)

	class _Iterator:
		def __init__(self, source: 'FileIterator'):
			self._source = source
			self._folders = None

		def __next__(self):
			if self._folders is None:
				self._folders = [[ self._source._path ]]

			while len(self._folders) > 0:
				parts = [ x[0] for x in self._folders ]
				path = self._source._from.join(parts)

				if self._source._from.is_dir(path):
					files = self._source._from.get_files(path)
					files.sort()

					self._folders.append(files)
					return path

				self._folders[-1].pop(0)
				while len(self._folders[-1]) == 0:
					self._folders.pop()
					if len(self._folders) > 0:
						self._folders[-1].pop(0)
					else:
						raise StopIteration

				return path
