# -*- coding: utf-8 -*-

class Reader:
	def __init__(self):
		pass

	def close(self) -> None:
		"""
		Close the reader and release any resources.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def forward(self, offset: int) -> int:
		"""
		Move the current position forward by a specified offset.

		Args:
			offset (int): The number of bytes to move forward.

		Returns:
			int: The new position.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def read(self, size: int = -1) -> bytes:
		"""
		Read a specified number of bytes from the source.

		Args:
			size (int): Number of bytes to read. Defaults to -1, which reads until EOF.

		Returns:
			bytes: The read bytes.
		"""
		raise NotImplementedError("Subclasses must implement this method.")

	def tell(self) -> int:
		"""
		Get the current position in the source.

		Returns:
			int: The current position.
		"""
		raise NotImplementedError("Subclasses must implement this method.")
