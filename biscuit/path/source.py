# -*- coding: utf-8 -*-

from typing import List
from .file_info import FileInfo

class Source:
	def __init__(self):
		pass

	def get_file_info(self, parent_directory: str, path: str) -> FileInfo:
		raise NotImplementedError("Subclasses must implement this method.")

	def get_files(self, directory: FileInfo) -> List[FileInfo]:
		raise NotImplementedError("Subclasses must implement this method.")

	def get_parent_directory(self, file: str) -> str:
		raise NotImplementedError("Subclasses must implement this method.")
