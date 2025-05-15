# -*- coding: utf-8 -*-

from typing import Dict, List

class Filter:
	def __init__(self, config: Dict, parent_filter: 'Filter' = None):
		self._exclude_if_present = set(x.get() for x in config['exclude_if_present'])
		if parent_filter is not None:
			self._exclude_if_present.union(parent_filter.get_exclude_if_present())
		self._exclude_other_filesystem = config['exclude_other_filesystem'].get_current()
		self._parent_filter = parent_filter

	def exclude_if_present(self, files: List[str]) -> bool:
		inter = self._exclude_if_present.intersection(files)
		return len(inter) > 0

	def get_exclude_if_present(self):
		return self._exclude_if_present

	def is_exclude_other_filesystem(self):
		if self._exclude_other_filesystem is None:
			return self._parent_option.is_exclude_other_filesystem()
		else:
			return self._exclude_other_filesystem
