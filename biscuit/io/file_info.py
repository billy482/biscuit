# -*- coding: utf-8 -*-

from os import stat_result
from typing import Any, Dict, Optional
from .reader import Reader
from .source import Source

ParentFileInfo = Optional['FileInfo']

class FileInfo:
	def __init__(self, info: Dict[str, Any], source: Source, parent: ParentFileInfo = None):
		self._filename = info['filename']
		self._path = info['path']
		self._mode = info['mode']
		self._dev = info['dev']
		self._uid = info['uid']
		self._gid = info['gid']
		self._size = info['size']
		self._mtime = info['mtime']
		self._parent = parent
		self._source = source
	
	def dev(self) -> int:
		"""
		Returns the device number associated with the file.

		Returns:
			int: The device number.
		"""
		return self._dev

	def filename(self) -> str:
		return self._filename

	@staticmethod
	def from_stat_result(filename: str, path: str, info: stat_result, source: Source, parent: ParentFileInfo = None) -> 'FileInfo':
		return FileInfo({
			'filename': filename,
			'path': path,
			'mode': info.st_mode,
			'dev': info.st_dev,
			'uid': info.st_uid,
			'gid': info.st_gid,
			'size': info.st_size,
			'mtime': info.st_mtime
		}, source, parent)

	def gid(self) -> int:
		return self._gid

	def is_dir(self) -> bool:
		return (self._mode & 0o40000) == 0o40000

	def is_file(self) -> bool:
		return (self._mode & 0o100000) == 0o100000

	def is_link(self) -> bool:
		return (self._mode & 0o120000) == 0o120000

	def __lt__(self, other: 'FileInfo') -> bool:
		return self._path < other._path
	
	def metadata(self) -> Dict[str, Any]:
		"""
		Returns the metadata of the file as a dictionary.

		Returns:
			Dict[str, Any]: A dictionary containing file metadata.
		"""
		return {
			'dev': self._dev,
			'gid': self._gid,
			'mode': self._mode,
			'mtime': self._mtime,
			'size': self._size,
			'uid': self._uid
		}

	def mode(self) -> int:
		return self._mode

	def mtime(self) -> float:
		return self._mtime

	def open_for_read(self) -> Reader:
		return self._source.open_for_read(self)

	def parent(self) -> ParentFileInfo:
		return self._parent
	
	def path(self) -> str:
		return self._path

	def read_link(self) -> str:
		return self._source.read_link(self)

	def __repr__(self) -> str:
		return f'FileInfo(path={self._path}, filename={self._filename}, mode={self._mode}, dev={self._dev}, uid={self._uid}, gid={self._gid}, size={self._size}, mtime={self._mtime})'

	def size(self) -> int:
		return self._size

	def uid(self) -> int:
		return self._uid
