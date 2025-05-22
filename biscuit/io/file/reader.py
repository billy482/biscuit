# -*- coding: utf-8 -*-

from ..file_info import FileInfo
from ..reader import Reader

class FileReader(Reader):
	"""
	FileReader is a subclass of Reader that reads data from a file.
	"""

	def __init__(self, file: FileInfo):
		"""
		Initialize the FileReader with the path to the file.

		Args:
			file_path (FileInfo): Path to the file to be read.
		"""
		super().__init__()
		self._file = open(file.path(), 'rb')

	def close(self) -> None:
		return self._file.close()

	def forward(self, offset: int) -> int:
		"""
		Move the current position forward by a specified offset.

		Args:
			offset (int): The number of bytes to move forward.

		Returns:
			int: The new position.
		"""
		return self._file.seek(offset, 1)
	
	def read(self, size: int = -1) -> bytes:
		"""
		Read a specified number of bytes from the source.

		Args:
			size (int): Number of bytes to read. Defaults to -1, which reads until EOF.

		Returns:
			bytes: The read bytes.
		"""
		return self._file.read(size)
	
	def tell(self) -> int:
		return self._file.tell()
