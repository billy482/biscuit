# -*- coding: utf-8 -*-

from os import stat_result
from typing import Any, Dict, Optional
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
		return (self._mode & 0o40000) != 0

	def __lt__(self, other: 'FileInfo') -> bool:
		return self._path < other._path
	
	def mode(self) -> int:
		return self._mode

	def mtime(self) -> float:
		return self._mtime

	def parent(self) -> ParentFileInfo:
		return self._parent
	
	def path(self) -> str:
		return self._path

	def __repr__(self) -> str:
		return f'FileInfo(path={self._path}, filename={self._filename}, mode={self._mode}, dev={self._dev}, uid={self._uid}, gid={self._gid}, size={self._size}, mtime={self._mtime})'

	def size(self) -> int:
		return self._size

	def uid(self) -> int:
		return self._uid
