# -*- coding: utf-8 -*-

from biscuit import Host
from typing import List
from .reader import Reader

class Source:
	def __init__(self):
		pass

	def get_file_info(self, path: str) -> 'FileInfo':
		"""
		Retrieves information about a file located at the specified path within the given parent directory.

		Args:
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

	def get_host(self) -> Host:
		"""
		Returns the host associated with this source.

		Returns:
		Host: An instance of Host representing the source's host.
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

	def read_link(self, file: 'FileInfo') -> str:
		"""
		Read the symbolic link of a file.

		Args:
			file (FileInfo): The file whose symbolic link is to be read.

		Returns:
			str: The target of the symbolic link.
		"""
		raise NotImplementedError("Subclasses must implement this method.")
