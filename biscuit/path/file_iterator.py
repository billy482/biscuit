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

			parent_directory = self._source._from.get_parent_directory(self._source._path) or self._source._path
			self._folders = [[ self._source._from.get_file_info(self._source._path, parent_directory) ]]

		def _move_to_next(self):
			self._folders[-1].pop(0)
			while len(self._folders[-1]) == 0:
				self._folders.pop()
				if len(self._folders) > 0:
					self._folders[-1].pop(0)
				else:
					raise StopIteration

		def __next__(self):
			while len(self._folders) > 0:
				file_info = self._folders[-1][0]

				if self._source._filter.is_excluded(file_info):
					self._move_to_next()
					continue

				if file_info.is_dir():
					files = self._source._from.get_files(file_info)

					if self._source._filter.exclude_if_present(files):
						self._move_to_next()
						continue

					self._folders.append(files)
					return file_info

				self._move_to_next()

				return file_info
