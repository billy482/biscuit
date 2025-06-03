# -*- coding: utf-8 -*-

class Host:
	"""
	Represents a host with a specified name.
	Attributes:
		_host_name (str): The name of the host.
	Methods:
		get_host_name() -> str:
			Returns the name of the host.
		localhost() -> Host:
			Creates and returns a Host instance representing the local host.
	"""
	def __init__(self, host_name: str) -> None:
		self._host_name = host_name

	def get_host_name(self) -> str:
		"""
		Return the name of the host.
		
		Returns:
			str: The name of the host.
		"""
		return self._host_name

	@staticmethod
	def localhost() -> 'Host':
		"""
		Create a Host instance for the local host.
		
		Returns:
			Host: An instance representing the local host.
		"""
		return Host("localhost")
