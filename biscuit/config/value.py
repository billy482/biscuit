# -*- coding: utf-8 -*-

class Value:
	"""
	A class to represent a value (and its default value) in the biscuit configuration.
	"""

	def __init__(self, value, default):
		"""
		Initialize the Value object.

		Args:
			value: The current value.
			default: The default value.
		"""
		self._value = value
		self._default = default

	def get(self):
		"""
		Get the current value.

		Returns:
			The current value.
		"""
		return self._value or self._default

	def get_current(self):
		"""
		Get the current value.

		Returns:
			The current value.
		"""
		return self._value
	
	def get_default(self):
		"""
		Get the default value.

		Returns:
			The default value.
		"""
		return self._default
