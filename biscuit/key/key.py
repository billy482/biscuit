# -*- coding: utf-8 -*-

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
import logging
from typing import Optional

strOpt = Optional[str]

class Key:
	def __init__(self, private_key_path: str):
		self._public_key = {
			'path': private_key_path + '.pub',
			'key': None
		}
		self._private_key = {
			'path': private_key_path,
			'key': None
		}

	def fingerprint(self) -> strOpt:
		"""
		Generates a fingerprint for the public key using SHA-256 hashing and Base64 encoding.

		Returns:
			Optional[str]: The Base64-encoded SHA-256 fingerprint of the public key if available, otherwise None.
		"""
		import base64
		from cryptography.hazmat.primitives import hashes

		if self._public_key['key'] is not None:
			public_key_bytes_der = self._public_key['key'].public_bytes(
				encoding = serialization.Encoding.DER,
				format = serialization.PublicFormat.SubjectPublicKeyInfo
			)
			hasher = hashes.Hash(hashes.SHA256())
			hasher.update(public_key_bytes_der)
			return base64.b16encode(hasher.finalize()).decode('utf-8')
		else:
			return None

	@staticmethod
	def generate_key_pair(private_key_path: str, key_length: int, passphrase: strOpt) -> 'Key':
		"""
		Generate a new RSA key pair and save them to files.

		Args:
			private_key_path (str): The file path where the private key will be saved.
			key_length (int): The length (in bits) of the RSA key to generate.
			passphrase (Optional[str]): Passphrase to encrypt the private key. If None, the private key is saved unencrypted.

		Side Effects:
			Writes the private key to `private_key_path` and the public key to `private_key_path + '.pub'`.

		Raises:
			ValueError: If key generation or file writing fails.
		"""
		from os.path import expanduser

		private_key_path = expanduser(private_key_path)

		logger = logging.getLogger('biscuit.keyring')
		logger.info(f"Generating new rsa key pair (path: {private_key_path}, length: {key_length})...")

		private_key = rsa.generate_private_key(
			public_exponent = 65537,
			key_size = key_length,
		)

		public_key = private_key.public_key()

		with open(private_key_path, 'wb') as f:
			encyption_algorithm = serialization.NoEncryption() if passphrase is None else serialization.BestAvailableEncryption(passphrase.encode())

			private_key_bytes = private_key.private_bytes(
				encoding = serialization.Encoding.PEM,
				format = serialization.PrivateFormat.TraditionalOpenSSL,
				encryption_algorithm = encyption_algorithm
			)
			f.write(private_key_bytes)
		logger.info(f"Private key saved to {private_key_path}")	

		with open(private_key_path + '.pub', 'wb') as f:
			public_key_bytes = public_key.public_bytes(
				encoding = serialization.Encoding.PEM,
				format = serialization.PublicFormat.SubjectPublicKeyInfo
			)
			f.write(public_key_bytes)
		logger.info(f"Public key saved to {private_key_path}.pub")

		new_key = Key(private_key_path)
		new_key._public_key['key'] = public_key
		new_key._private_key['key'] = private_key

		return new_key

	def load_public_key(self) -> bool:
		"""
		Load the public key from the file.

		Returns:
			bool: True if the public key was loaded successfully, False otherwise.
		"""
		logger = logging.getLogger('biscuit.keyring')
		logger.info(f"Loading public key from {self._public_key['path']}...")

		with open(self._public_key['path'], 'rb') as f:
			public_key_bytes = f.read()

		try:
			self._public_key['key'] = serialization.load_pem_public_key(public_key_bytes)
			logger.info("Public key loaded successfully.")
			return True
		except ValueError as e:
			logger.error(f"Failed to load public key: {e}")
			return False

	def load_private_key(self, passphrase: strOpt = None) -> bool:
		"""
		Loads a private key from the file specified in self._private_key['path'].

		Attempts to deserialize the private key using the provided passphrase.
		If the passphrase is None, the key is assumed to be unencrypted.

		Args:
			passphrase (Optional[str]): The passphrase to decrypt the private key, or None if the key is unencrypted.

		Returns:
			bool: True if the private key was loaded successfully, False if loading or decryption failed.
		"""
		logger = logging.getLogger('biscuit.keyring')
		logger.info(f"Loading private key from {self._private_key['path']}...")

		with open(self._private_key['path'], 'rb') as f:
			private_key_bytes = f.read()

		try:
			self._private_key['key'] = serialization.load_pem_private_key(
				private_key_bytes,
				password = passphrase.encode() if passphrase is not None else None
			)
			logger.info("Private key loaded successfully.")
			return True
		except ValueError as e:
			logger.error(f"Failed to load private key: {e}")
			return False
