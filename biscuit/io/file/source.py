# -*- coding: utf-8 -*-

from biscuit import Host
import os
from typing import Dict, List
from ..file_info import FileInfo
from ..reader import Reader
from ..source import Source

class FileSource(Source):
	def __init__(self, config: Dict):
		self._path = config['path'].get()

	def get_file_info(self, parent_directory: str, path: str) -> FileInfo:
		stat = os.stat(path)
		return FileInfo.from_stat_result(parent_directory, path, stat, self)

	def get_files(self, directory: FileInfo) -> List[FileInfo]:
		files = os.scandir(directory.path())
		return sorted(map(lambda x: FileInfo.from_stat_result(x.name, x.path, x.stat(), self, directory), files))

	def get_host(self) -> Host:
		return Host.localhost()

	def get_parent_directory(self, file: str) -> str:
		return os.path.dirname(file)

	def open_for_read(self, file: FileInfo) -> Reader:
		from .reader import FileReader
		return FileReader(file)
