# -*- coding: utf-8 -*-

import os
from typing import Dict
from ..filter import Filter

class FileIterator():
	def __init__(self, config: Dict):
		self._path = config['path'].get()
		self._folders = [[self._path]]

	def __iter__(self):
		return self

	def __next__(self):
		if len(self._folders) == 0:
			return None

		path = os.path.join(*[ x[0] for x in self._folders ])

		while len(self._folders) > 0:
			# TODO: check if the file is on the same device as parent folder

			if os.path.isdir(path):
				sub_files = os.listdir(path)
				sub_files.sort()

				if not self.option.exclude_if_present(sub_files):
					self._folders.append(sub_files)
					return path

			self._folders[-1].pop(0)
			while len(self._folders[-1]) == 0:
				self._folders.pop()
				if len(self._folders) > 0:
					self._folders[-1].pop(0)
				else:
					return None

			return path

		return None
