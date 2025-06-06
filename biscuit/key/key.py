# -*- coding: utf-8 -*-

from asn1crypto import cms, algos, core, x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa
import logging
from os import urandom
from typing import Any, Dict, Optional, Tuple

strOpt = Optional[str]

class Key:
	def __init__(self, config: Dict[str, Any], private_key_path: strOpt = None):
		if private_key_path is None:
			private_key_path = config['key']['path'].get()
		if private_key_path is None:
			private_key_path = 'key'

		from os.path import expanduser
		private_key_path = expanduser(private_key_path)

		self._cipher = config['key']['cipher'].get()
		self._public_key = {
			'fingerprint': {},
			'path': private_key_path + '.pub',
			'key': None
		}
		self._private_key = {
			'path': private_key_path,
			'key': None
		}
		self._logger = logging.getLogger('biscuit.keyring')

	def decrypt(self, data: bytes, passphrase: strOpt = None) -> bytes:
		"""
		Decrypts CMS EnvelopedData using the private key and specified passphrase.

		This method loads the private key if not already loaded, parses the CMS structure,
		decrypts the session key using RSA OAEP, and then decrypts the content using the
		appropriate symmetric algorithm (AES-GCM, AES-CCM, or ChaCha20Poly1305) based on
		the encryption algorithm OID.

		Args:
			data (bytes): The CMS EnvelopedData to decrypt.
			passphrase (Optional[str]): The passphrase to unlock the private key, if required.

		Returns:
			bytes: The decrypted content.

		Raises:
			ValueError: If the CMS message is not of type EnvelopedData.
			KeyError: If the encryption algorithm is not supported.
			Exception: If decryption fails due to invalid key, passphrase, or corrupted data.
		"""
		if self._private_key['key'] is None:
			self.load_private_key(passphrase)

		content_info = cms.ContentInfo.load(data)

		if content_info['content_type'].native != 'enveloped_data':
			raise ValueError("The CMS message is not of type EnvelopedData.")

		enveloped_data = content_info['content']
		recipient_info = enveloped_data['recipient_infos'][0]

		encrypted_key = recipient_info.chosen['encrypted_key'].native
		key = self._private_key['key'].decrypt(
			encrypted_key,
			padding.OAEP(
				mgf = padding.MGF1(algorithm = hashes.SHA256()),
				algorithm = hashes.SHA256(),
				label = None
			)
		)

		encrypted_content_info = enveloped_data['encrypted_content_info']
		content_encryption_algorithm = encrypted_content_info['content_encryption_algorithm']
		encrypted_content = encrypted_content_info['encrypted_content'].native

		iv = content_encryption_algorithm['parameters'].native


		def decrypt_aes_ccm(key: bytes, iv: bytes, aad: bytes, encrypted_data: bytes) -> bytes:
			from cryptography.hazmat.primitives.ciphers.aead import AESCCM

			cipher = AESCCM(key)
			return cipher.decrypt(iv, encrypted_data, aad)

		def decrypt_aes_gcm(key: bytes, iv: bytes, aad: bytes, encrypted_data: bytes) -> bytes:
			from cryptography.hazmat.primitives.ciphers.aead import AESGCM

			cipher = AESGCM(key)
			return cipher.decrypt(iv, encrypted_data, aad)

		def decrypt_chacha20poly1305(key: bytes, nonce: bytes, aad: bytes, encrypted_data: bytes) -> bytes:
			from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305

			cipher = ChaCha20Poly1305(key)
			return cipher.decrypt(nonce, encrypted_data, aad)

		algorithms = {
			'aes128_ccm': {
				'fonction': decrypt_aes_ccm,
				'name': 'AES128-CCM'
			},
			'aes256_ccm': {
				'fonction': decrypt_aes_ccm,
				'name': 'AES256-CCM'
			},
			'aes128_gcm': {
				'fonction': decrypt_aes_gcm,
				'name': 'AES128-GCM'
			},
			'aes192_gcm': {
				'fonction': decrypt_aes_gcm,
				'name': 'AES192-GCM'
			},
			'aes256_gcm': {
				'fonction': decrypt_aes_gcm,
				'name': 'AES256-GCM'
			},

			'1.2.840.113549.1.9.16.3.18': {
				'fonction': decrypt_chacha20poly1305,
				'name': 'ChaCha20Poly1305'
			},
			'2.16.840.1.101.3.4.1.6': {
				'fonction': decrypt_aes_gcm,
				'name': 'AES128-GCM'
			},
			'2.16.840.1.101.3.4.1.7': {
				'fonction': decrypt_aes_ccm,
				'name': 'AES128-CCM'
			},
			'2.16.840.1.101.3.4.1.26': {
				'fonction': decrypt_aes_gcm,
				'name': 'AES192-GCM'
			},
			'2.16.840.1.101.3.4.1.46': {
				'fonction': decrypt_aes_gcm,
				'name': 'AES256-GCM'
			},
			'2.16.840.1.101.3.4.1.47': {
				'fonction': decrypt_aes_ccm,
				'name': 'AES256-CCM'
			}
		}

		oid = content_encryption_algorithm['algorithm'].native
		aad = f'Biscuit Encrypted Data: {self.fingerprint("sha256")}'.encode('utf-8')
		return algorithms[oid]['fonction'](key, iv, aad, encrypted_content)

	def encrypt(self, data: bytes) -> bytes:
		"""
		Encrypts the given data using a randomly generated symmetric key and the selected cipher algorithm.
		The symmetric key is then encrypted with the loaded public RSA key using OAEP padding.
		The result is returned as a CMS EnvelopedData structure.

		Supported cipher algorithms include:
			- AES128-CCM
			- AES256-CCM
			- AES128-GCM
			- AES192-GCM
			- AES256-GCM
			- ChaCha20Poly1305

		The Additional Authenticated Data (AAD) includes a fingerprint of the public key for integrity.

		Args:
			data (bytes): The plaintext data to encrypt.

		Returns:
			bytes: The DER-encoded CMS EnvelopedData structure containing the encrypted content and encrypted symmetric key.

		Raises:
			KeyError: If the selected cipher algorithm is not supported.
			Exception: If the public key is not loaded or encryption fails.
		"""
		if self._public_key['key'] is None:
			self.load_public_key()

		def encrypt_aes128_ccm(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import AESCCM

			key = AESCCM.generate_key(bit_length = 128)
			iv = urandom(12)  # AES GCM uses a 12-byte nonce

			cipher = AESCCM(key)
			ct = cipher.encrypt(iv, data, aad)

			return key, iv, ct

		def encrypt_aes256_ccm(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import AESCCM

			key = AESCCM.generate_key(bit_length = 256)
			iv = urandom(12)  # AES GCM uses a 12-byte nonce

			cipher = AESCCM(key)
			ct = cipher.encrypt(iv, data, aad)

			return key, iv, ct

		def encrypt_aes128_gcm(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import AESGCM

			key = AESGCM.generate_key(bit_length = 128)
			iv = urandom(12)  # AES GCM uses a 12-byte nonce

			cipher = AESGCM(key)
			ct = cipher.encrypt(iv, data, aad)

			return key, iv, ct

		def encrypt_aes192_gcm(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import AESGCM

			key = AESGCM.generate_key(bit_length = 192)
			iv = urandom(12)  # AES GCM uses a 12-byte nonce

			cipher = AESGCM(key)
			ct = cipher.encrypt(iv, data, aad)

			return key, iv, ct

		def encrypt_aes256_gcm(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import AESGCM

			key = AESGCM.generate_key(bit_length = 256)
			iv = urandom(12)  # AES GCM uses a 12-byte nonce

			cipher = AESGCM(key)
			ct = cipher.encrypt(iv, data, aad)

			return key, iv, ct

		def encrypt_chacha20poly1305(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305

			key = ChaCha20Poly1305.generate_key()
			nonce = urandom(12)  # ChaCha20 uses a 12-byte nonce

			cipher = ChaCha20Poly1305(key)
			ct = cipher.encrypt(nonce, data, aad)

			return key, nonce, ct

		algorithms = {
			'AES128-CCM': {
				'fonction': encrypt_aes128_ccm,
				'oid': 'aes128_ccm'
			},
			'AES256-CCM': {
				'fonction': encrypt_aes256_ccm,
				'oid': 'aes256_ccm'
			},
			'AES128-GCM': {
				'fonction': encrypt_aes128_gcm,
				'oid': 'aes128_gcm'
			},
			'AES192-GCM': {
				'fonction': encrypt_aes192_gcm,
				'oid': 'aes192_gcm'
			},
			'AES128-GCM': {
				'fonction': encrypt_aes128_gcm,
				'oid': 'aes128_gcm'
			},
			'AES256-GCM': {
				'fonction': encrypt_aes256_gcm,
				'oid': 'aes256_gcm'
			},
			'ChaCha20Poly1305': {
				'fonction': encrypt_chacha20poly1305,
				'oid': '1.2.840.113549.1.9.16.3.18'
			}
		}

		aad = f'Biscuit Encrypted Data: {self.fingerprint("sha256")}'.encode('utf-8')
		key, iv, encrypted_data = algorithms[self._cipher]['fonction'](aad, data)

		encrypted_key = self._public_key['key'].encrypt(
			key,
			padding.OAEP(
				mgf = padding.MGF1(hashes.SHA256()),
				algorithm = hashes.SHA256(),
				label = None
			)
		)

		enveloped_data = cms.EnvelopedData({
			'version': 'v0',
			'recipient_infos': [
				cms.RecipientInfo(name = 'ktri', value = cms.KeyTransRecipientInfo({
					'version': 'v0',
					'rid': cms.RecipientIdentifier(name = 'issuer_and_serial_number', value = cms.IssuerAndSerialNumber({
						'issuer': x509.Name.build({'common_name': 'Biscuit'}),
						'serial_number': 1
					})),
					'key_encryption_algorithm': cms.KeyEncryptionAlgorithm({
						'algorithm': 'rsaes_oaep',
						'parameters': algos.RSAESOAEPParams({
							'hash_algorithm': algos.DigestAlgorithm({'algorithm': 'sha256'}),
							'mask_gen_algorithm': algos.MaskGenAlgorithm({
								'algorithm': 'mgf1',
								'parameters': algos.DigestAlgorithm({'algorithm': 'sha256'})
							}),
							'p_source_algorithm': algos.PSourceAlgorithm({
								'algorithm': '1.2.840.113549.1.1.9',
								'parameters': core.OctetString(b'')
							})
						})
					}),
					'encrypted_key': encrypted_key
				}))
			],
			'encrypted_content_info': cms.EncryptedContentInfo({
				'content_type': 'data',
				'content_encryption_algorithm': algos.EncryptionAlgorithm({
					'algorithm': algorithms[self._cipher]['oid'],
					'parameters': core.OctetString(iv)
				}),
				'encrypted_content': core.OctetString(encrypted_data)
			})
		})

		content_info = cms.ContentInfo({
			'content_type': 'enveloped_data',
			'content': enveloped_data
		})

		return content_info.dump()

	def fingerprint(self, algo: str = 'sha256') -> strOpt:
		"""
		Generates and returns the fingerprint of the public key using the specified hashing algorithm.

		If the fingerprint for the given algorithm is already computed and cached, it is returned directly.
		Otherwise, the fingerprint is computed by serializing the public key to DER format, hashing it with
		the specified algorithm, and encoding the result in base16 (hexadecimal). The computed fingerprint
		is then cached and returned.

		Args:
			algo (str, optional): The hashing algorithm to use for the fingerprint. Supported values are
				'md5', 'sha1', 'sha256', and 'sha512'. Defaults to 'sha256'.

		Returns:
			Optional[str]: The fingerprint of the public key as a base16-encoded string, or None if the
			public key is not loaded.

		Raises:
			KeyError: If an unsupported algorithm is specified.
		"""
		import base64

		if algo in self._public_key['fingerprint']:
			return self._public_key['fingerprint'][algo]

		if self._public_key['key'] is not None:
			public_key_bytes_der = self._public_key['key'].public_bytes(
				encoding = serialization.Encoding.DER,
				format = serialization.PublicFormat.SubjectPublicKeyInfo
			)

			algos = {
				'md5': hashes.MD5,
				'sha1': hashes.SHA1,
				'sha256': hashes.SHA256,
				'sha512': hashes.SHA512,
			}

			hasher = hashes.Hash(algos[algo]())
			hasher.update(public_key_bytes_der)
			self._public_key['fingerprint'][algo] = base64.b16encode(hasher.finalize()).decode('utf-8')

			return self._public_key['fingerprint'][algo]
		else:
			return None

	@staticmethod
	def generate_key_pair(config: Dict[str, Any], private_key_path: str, key_length: int, passphrase: strOpt) -> 'Key':
		"""
		Generates a new RSA key pair and returns a Key object containing the generated keys.

		Args:
			config (Dict[str, Any]): Configuration dictionary for the Key object.
			private_key_path (str): Path where the private key will be stored (can include '~' for home directory).
			key_length (int): Length of the RSA key to generate, in bits.
			passphrase (strOpt): Optional passphrase to encrypt the private key.

		Returns:
			Key: A Key object containing the generated RSA private and public keys.

		Logs:
			Info-level log indicating the path and length of the generated key pair.
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

		new_key = Key(config, private_key_path)
		new_key._public_key['key'] = public_key
		new_key._private_key['key'] = private_key

		return new_key

	def key_length(self) -> int:
		"""
		Get the length of the private key in bits.

		Returns:
			int: The length of the private key in bits, or 0 if the key is not loaded.
		"""
		if self._private_key['key'] is not None:
			return self._private_key['key'].key_size
		elif self._public_key['key'] is not None:
			return self._public_key['key'].key_size
		else:
			return 0

	def load_public_key(self) -> None:
		"""
		Loads a public key from the file path specified in the `_public_key` dictionary.

		Reads the public key file in PEM format, deserializes it, and stores the loaded key
		in the `_public_key['key']` entry. Logs the loading process and handles errors for
		missing files and invalid key formats.

		Raises:
			FileNotFoundError: If the public key file does not exist.
			ValueError: If the public key cannot be deserialized from the file.
		"""
		self._logger.info(f"Loading public key from {self._public_key['path']}...")

		try:
			with open(self._public_key['path'], 'rb') as f:
				public_key_bytes = f.read()

			self._public_key['key'] = serialization.load_pem_public_key(public_key_bytes)
			self._logger.info("Public key loaded successfully.")

		except FileNotFoundError as e:
			self._logger.error(f"Private key file not found: {e}")
			raise

		except ValueError as e:
			self._logger.error(f"Failed to load public key: {e}")
			raise

	def load_private_key(self, passphrase: strOpt = None) -> None:
		"""
		Loads a private key from the file specified in self._private_key['path'].

		Attempts to read the private key file in PEM format and loads it using the provided passphrase.
		The loaded key is stored in self._private_key['key'].

		Args:
			passphrase (Optional[str]): The passphrase to decrypt the private key, if it is encrypted.
				If None, assumes the key is not encrypted.

		Raises:
			FileNotFoundError: If the private key file does not exist.
			ValueError: If the private key cannot be loaded (e.g., due to incorrect passphrase or invalid file format).
		"""
		self._logger.info(f"Loading private key from {self._private_key['path']}...")

		try:
			with open(self._private_key['path'], 'rb') as f:
				private_key_bytes = f.read()

			self._private_key['key'] = serialization.load_pem_private_key(
				private_key_bytes,
				password = passphrase.encode() if passphrase is not None else None
			)
			self._logger.info("Private key loaded successfully.")

		except FileNotFoundError as e:
			self._logger.error(f"Private key file not found: {e}")
			raise

		except ValueError as e:
			self._logger.error(f"Failed to load private key: {e}")
			raise

	def save_public_key(self, path: strOpt = None) -> None:
		"""
		Saves the public key to the specified path or to the default path if none is provided.

		Args:
		path (Optional[str]): The file path where the public key will be saved. If None, uses the default path.

		Returns:
		bool: True if the public key was saved successfully, False otherwise.
		"""
		if path is None:
			path = self._public_key['path']

		self._logger.info(f"Saving public key to {path}...")

		try:
			public_key_bytes = self._public_key['key'].public_bytes(
				encoding = serialization.Encoding.PEM,
				format = serialization.PublicFormat.SubjectPublicKeyInfo
			)
			with open(path, 'wb') as f:
				f.write(public_key_bytes)
			self._logger.info("Public key saved successfully.")

		except Exception as e:
			self._logger.error(f"Failed to save public key: {e}")
			raise

	def save_private_key(self, path: strOpt = None, passphrase: strOpt = None) -> bool:
		"""
		Saves the private key to the specified path or to the default path if none is provided.

		Args:
		path (Optional[str]): The file path where the private key will be saved. If None, uses the default path.
		passphrase (Optional[str]): The passphrase to encrypt the private key. If None, the key is saved unencrypted.

		Returns:
		bool: True if the private key was saved successfully, False otherwise.
		"""
		if path is None:
			path = self._private_key['path']

		self._logger.info(f"Saving private key to {path}...")

		if passphrase is None:
			encryption_algorithm = serialization.NoEncryption()
		else:
			encryption_algorithm = serialization.BestAvailableEncryption(passphrase.encode())

		private_key_bytes = self._private_key['key'].private_bytes(
			encoding = serialization.Encoding.PEM,
			format = serialization.PrivateFormat.TraditionalOpenSSL,
			encryption_algorithm = encryption_algorithm
		)

		try:
			with open(path, 'wb') as f:
				f.write(private_key_bytes)
			self._logger.info("Private key saved successfully.")
			return True

		except Exception as e:
			self._logger.error(f"Failed to save private key: {e}")
			return False
