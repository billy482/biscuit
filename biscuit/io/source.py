# -*- coding: utf-8 -*-

from typing import List
from .reader import Reader

class Source:
	def __init__(self):
		pass

	def get_file_info(self, parent_directory: str, path: str) -> 'FileInfo':
		"""
		Retrieves information about a file located at the specified path within the given parent directory.

		Args:
			parent_directory (str): The path to the parent directory containing the file.
			path (str): The relative or absolute path to the file whose information is to be retrieved.

		Returns:
			FileInfo: An object containing metadata and details about the specified file.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def get_files(self, directory: 'FileInfo') -> List['FileInfo']:
		"""
		Retrieve a list of files from the specified directory.

		Args:
			directory (FileInfo): The directory from which to retrieve files.

		Returns:
			List[FileInfo]: A list of FileInfo objects representing the files in the directory.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def get_parent_directory(self, file: str) -> str:
		"""
		Returns the parent directory of the given file path.

		Args:
			file (str): The path to the file.
			str: The parent directory of the specified file path.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def open_for_read(self, file: 'FileInfo') -> Reader:
		"""
		Open a file for reading.

		Args:
			file (FileInfo): The file to be opened.

		Returns:
			Reader: An instance of Reader for reading the file.
		"""
		raise NotImplementedError("Subclasses must implement this method.")
