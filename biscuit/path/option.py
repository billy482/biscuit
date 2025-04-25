# -*- coding: utf-8 -*-

from typing import Dict

class Option:
	def __init__(self, config: Dict, parent_option: 'Option' = None):
		self.exclude_if_present = [ x.get() for x in config['exclude_if_present'] ]
		if parent_option is not None:
			self.exclude_if_present.extend(parent_option.get_exclude_if_present())
		self.exclude_if_present.sort()
		self.exclude_other_filesystem = config['exclude_other_filesystem'].get_current()
		self.parent_option = parent_option

	def get_exclude_if_present(self):
		return self.exclude_if_present

	def is_exclude_other_filesystem(self):
		if self.exclude_other_filesystem is None:
			return self.parent_option.is_exclude_other_filesystem()
		else:
			return self.exclude_other_filesystem
