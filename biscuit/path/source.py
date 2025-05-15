# -*- coding: utf-8 -*-

from typing import List

class Source:
	def __init__(self):
		pass

	def get_files(self, path: str) -> List[str]:
		raise NotImplementedError("Subclasses must implement this method.")

	def is_dir(self, path: str) -> bool:
		raise NotImplementedError("Subclasses must implement this method.")

	def join(self, path: List[str]) -> str:
		raise NotImplementedError("Subclasses must implement this method.")
