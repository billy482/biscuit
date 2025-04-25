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
		self.value = value
		self.default = default

	def get(self):
		"""
		Get the current value.

		Returns:
			The current value.
		"""
		return self.value or self.default

	def get_current(self):
		"""
		Get the current value.

		Returns:
			The current value.
		"""
		return self.value
	
	def get_default(self):
		"""
		Get the default value.

		Returns:
			The default value.
		"""
		return self.default
