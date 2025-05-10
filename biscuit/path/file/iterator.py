# -*- coding: utf-8 -*-

import os
from typing import Dict
from ..iterator import Iterator

class FileIterator(Iterator):
	def __init__(self, config: Dict):
		self.path = config['path'].get()
		self.folders = [[self.path]]

	def __iter__(self):
		return self

	def __next__(self):
		if len(self.folders) == 0:
			return None

		path = os.path.join(*[ x[0] for x in self.folders ])

		while len(self.folders) > 0:
			# TODO: check if the file is on the same device as parent folder

			if os.path.isdir(path):
				sub_files = os.listdir(path)
				sub_files.sort()
				self.folders.append(sub_files)
			else:
				self.folders[-1].pop(0)
				while len(self.folders[-1]) == 0:
					self.folders.pop()
					if len(self.folders) > 0:
						self.folders[-1].pop(0)
					else:
						return None

			return path

		return None
