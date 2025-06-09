# -*- coding: utf-8 -*-

from biscuit import Host
from biscuit.io import FileInfo, Reader, Source
import os
from os.path import basename, join
from typing import Dict, List

class FileSource(Source):
	def __init__(self, config: Dict):
		self._path = config['path'].get()

	def get_file_info(self, path: str) -> FileInfo:
		stat = os.lstat(path)
		parent_directory = self.get_parent_directory(path)
		return FileInfo.from_stat_result(basename(path), join(parent_directory, path), stat, self)

	def get_files(self, directory: FileInfo) -> List[FileInfo]:
		files = os.scandir(directory.path())
		return sorted(map(lambda x: FileInfo.from_stat_result(x.name, x.path, os.lstat(x.path), self), files))

	def get_host(self) -> Host:
		return Host.localhost()

	def get_parent_directory(self, file: str) -> str:
		return os.path.dirname(file)

	def open_for_read(self, file: FileInfo) -> Reader:
		from .reader import FileReader
		return FileReader(file)

	def read_link(self, file: FileInfo) -> str:
		return os.readlink(file.path())
