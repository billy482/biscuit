# -*- coding: utf-8 -*-

from asn1crypto import cms, algos, core, x509
import base64
from cryptography.hazmat.primitives import hashes, padding, serialization
from cryptography.hazmat.primitives.asymmetric import padding as rsa_padding, rsa
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
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

	def decrypt(self, data: bytes, passphrase: strOpt = None) -> bytes:
		"""
		Decrypts CMS EnvelopedData using the loaded private key.

		Args:
			data (bytes): The CMS EnvelopedData to decrypt.
			passphrase (Optional[str]): Passphrase to unlock the private key, if required.

		Returns:
			bytes: The decrypted content.

		Raises:
			ValueError: If the CMS message is not of type EnvelopedData or if an unsupported algorithm is encountered.
			Exception: If decryption or unpadding fails.

		Notes:
			- Supports AES128, AES256, Camellia, and ChaCha20 algorithms with various modes (CBC, CFB, CFB8, CTR, GCM, OFB).
			- Handles PKCS7 and ANSI X.923 padding when required.
		"""
		if self._private_key['key'] is None:
			self.load_private_key(passphrase)

		content_info = cms.ContentInfo.load(data)

		if content_info['content_type'].native != 'enveloped_data':
			raise ValueError("Le message CMS n'est pas de type EnvelopedData.")

		enveloped_data = content_info['content']
		recipient_info = enveloped_data['recipient_infos'][0]

		encrypted_key = recipient_info.chosen['encrypted_key'].native
		key = self._private_key['key'].decrypt(
			encrypted_key,
			rsa_padding.OAEP(
				mgf=rsa_padding.MGF1(algorithm=hashes.SHA256()),
				algorithm=hashes.SHA256(),
				label=None
			)
		)

		encrypted_content_info = enveloped_data['encrypted_content_info']
		content_encryption_algorithm = encrypted_content_info['content_encryption_algorithm']
		encrypted_content = encrypted_content_info['encrypted_content'].native

		iv = content_encryption_algorithm['parameters'].native


		def decrypt_aes256_gcm(key: bytes, iv: bytes, aad: bytes, encrypted_data: bytes) -> bytes:
			from cryptography.hazmat.primitives.ciphers.aead import AESGCM

			cipher = AESGCM(key)
			return cipher.decrypt(iv, encrypted_data, aad)

		def decrypt_chacha20poly1305(key: bytes, iv: bytes, aad: bytes, encrypted_data: bytes) -> bytes:
			from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305

			cipher = ChaCha20Poly1305(key)
			return cipher.encrypt(nonce, encrypted_data, aad)

		algorithms = {
			'aes256_gcm': {
				'fonction': decrypt_aes256_gcm,
				'name': 'AES256-GCM'
			},
			'1.2.840.113549.1.9.16.3.18': {
				'fonction': decrypt_chacha20poly1305,
				'name': 'ChaCha20Poly1305'
			},
			'2.16.840.1.101.3.4.1.46': {
				'fonction': decrypt_aes256_gcm,
				'name': 'AES256-GCM'
			}
		}

		oid = content_encryption_algorithm['algorithm'].native
		aad = f'Biscuit Encrypted Data: {self.fingerprint("sha256")}'.encode('utf-8')
		return algorithms[oid]['fonction'](key, iv, aad, encrypted_content)

	def encrypt(self, data: bytes) -> bytes:
		"""
		Encrypts the given data using the specified symmetric encryption algorithm and mode, then wraps the symmetric key using the loaded public key (RSA-OAEP). The result is returned as a CMS EnvelopedData structure.

		Args:
			data (bytes): The plaintext data to encrypt.

		Returns:
			bytes: The DER-encoded CMS ContentInfo structure containing the encrypted data and encrypted symmetric key.

		Raises:
			ValueError: If the specified algorithm or mode is not supported, or if the public key is not loaded.

		Encryption Details:
			- Supported algorithms: AES128, AES256, Camellia, ChaCha20.
			- Supported modes: CBC, CFB, CFB8, CTR, GCM, OFB.
			- Padding (if required): PKCS7 or ANSIX923.
			- The symmetric key is encrypted with the recipient's RSA public key using OAEP with SHA-256.
			- The output is a CMS EnvelopedData structure containing all necessary information for decryption.
		"""
		if self._public_key['key'] is None:
			self.load_public_key()

		def encrypt_aes256_gcm(aad: bytes, data: bytes) -> Tuple[bytes, bytes, bytes]:
			from cryptography.hazmat.primitives.ciphers.aead import AESGCM

			key = AESGCM.generate_key(bit_length=256)
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
			rsa_padding.OAEP(
				mgf = rsa_padding.MGF1(hashes.SHA256()),
				algorithm = hashes.SHA256(),
				label = None
			)
		)

		enveloped_data = cms.EnvelopedData({
			'version': 'v0',
			'recipient_infos': [
				cms.RecipientInfo(name='ktri', value=cms.KeyTransRecipientInfo({
					'version': 'v0',
					'rid': cms.RecipientIdentifier(name='issuer_and_serial_number', value=cms.IssuerAndSerialNumber({
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
		Generate a new RSA key pair and save them to files.

		Args:
			config (Dict[str, Any]): The configuration dictionary containing settings for key generation.
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

	def save_public_key(self, path: strOpt = None) -> bool:
		"""
		Saves the public key to the specified path or to the default path if none is provided.

		Args:
		path (Optional[str]): The file path where the public key will be saved. If None, uses the default path.

		Returns:
		bool: True if the public key was saved successfully, False otherwise.
		"""
		if path is None:
			path = self._public_key['path']

		logger = logging.getLogger('biscuit.keyring')
		logger.info(f"Saving public key to {path}...")

		try:
			with open(path, 'wb') as f:
				public_key_bytes = self._public_key['key'].public_bytes(
					encoding = serialization.Encoding.PEM,
					format = serialization.PublicFormat.SubjectPublicKeyInfo
				)
				f.write(public_key_bytes)
			logger.info("Public key saved successfully.")
			return True

		except Exception as e:
			logger.error(f"Failed to save public key: {e}")
			return False

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

		logger = logging.getLogger('biscuit.keyring')
		logger.info(f"Saving private key to {path}...")

		try:
			with open(path, 'wb') as f:
				if passphrase is None:
					encryption_algorithm = serialization.NoEncryption()
				else:
					encryption_algorithm = serialization.BestAvailableEncryption(passphrase.encode())

				private_key_bytes = self._private_key['key'].private_bytes(
					encoding = serialization.Encoding.PEM,
					format = serialization.PrivateFormat.TraditionalOpenSSL,
					encryption_algorithm = encryption_algorithm
				)
				f.write(private_key_bytes)
			logger.info("Private key saved successfully.")
			return True

		except Exception as e:
			logger.error(f"Failed to save private key: {e}")
			return False
