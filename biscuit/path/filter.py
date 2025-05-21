# -*- coding: utf-8 -*-

from typing import Dict, List, Optional
from .file_info import FileInfo

class Filter:
	def __init__(self, config: Dict, parent_filter: Optional['Filter'] = None):
		import re

		def compile(pattern: str) -> re.Pattern:
			import fnmatch
			reg_ex = fnmatch.translate(pattern)
			return re.compile(reg_ex)

		self._exclude = set(compile(x.get()) for x in config['exclude'])
		if parent_filter is not None:
			self._exclude.union(parent_filter.get_exclude())

		self._exclude_if_present = set(x.get() for x in config['exclude_if_present'])
		if parent_filter is not None:
			self._exclude_if_present.union(parent_filter.get_exclude_if_present())

		self._exclude_other_filesystem = config['exclude_other_filesystem'].get_current()

		self._parent_filter = parent_filter

	def exclude_if_present(self, files: List[FileInfo]) -> bool:
		inter = self._exclude_if_present.intersection(map(lambda x: x.filename(), files))
		return len(inter) > 0

	def get_exclude(self):
		return self._exclude_if_present

	def get_exclude_if_present(self):
		return self._exclude_if_present

	def is_excluded(self, file_info: FileInfo) -> bool:
		if self._parent_filter is not None:
			if self._parent_filter.is_excluded(file_info):
				return True

		for pattern in self._exclude:
			if pattern.match(file_info.path()):
				return True

		return False
#
#	def is_exclude_other_filesystem(self):
#		if self._exclude_other_filesystem is None:
#			return self._parent_option.is_exclude_other_filesystem()
#		else:
#			return self._exclude_other_filesystem
