# -*- coding: utf-8 -*-

class Host:
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
