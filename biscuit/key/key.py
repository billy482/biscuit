# -*- coding: utf-8 -*-

from asn1crypto import cms, algos, core, x509
import base64
from cryptography.hazmat.primitives import hashes, padding, serialization
from cryptography.hazmat.primitives.asymmetric import padding as rsa_padding, rsa
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
import logging
from os import urandom
from typing import Any, Dict, Optional

strOpt = Optional[str]

class Key:
	def __init__(self, config: Dict[str, Any], private_key_path: strOpt = None):
		if private_key_path is None:
			private_key_path = config['key']['path'].get()
		if private_key_path is None:
			private_key_path = 'key'

		self._algo = config['key']['algo'].get()
		self._mode = config['key']['mode'].get()
		self._padding = config['key']['padding'].get()
		self._public_key = {
			'fingerprint': {},
			'path': private_key_path + '.pub',
			'key': None
		}
		self._private_key = {
			'path': private_key_path,
			'key': None
		}

	def encrypt(self, data: bytes) -> bytes:
		"""
		Encrypts the given data using the configured symmetric algorithm and mode, then wraps the symmetric key using the recipient's public RSA key with OAEP padding, and finally packages everything into a CMS EnvelopedData structure.

		Args:
			data (bytes): The plaintext data to encrypt.

		Returns:
			bytes: The DER-encoded CMS ContentInfo structure containing the encrypted data and encrypted symmetric key.

		Raises:
			ValueError: If the required public key is not loaded or if an unsupported algorithm, mode, or padding is specified.

		Process:
			1. Loads the recipient's public key if not already loaded.
			2. Generates a random symmetric key and IV based on the selected algorithm (AES128 or AES256).
			3. Applies the selected block cipher mode (currently supports CBC).
			4. Applies the specified padding scheme if required (PKCS7 or ANSIX923).
			5. Encrypts the data with the symmetric key.
			6. Encrypts the symmetric key with the recipient's public RSA key using OAEP.
			7. Constructs a CMS EnvelopedData structure containing the encrypted content and encrypted key.
			8. Returns the DER-encoded CMS ContentInfo.
		"""
		if self._public_key['key'] is None:
			self.load_public_key()

		algo_name = []
		if self._algo == 'AES128':
			key = urandom(16)
			iv = urandom(16)
			algo = algorithms.AES128(key)
			algo_name.append('aes128')
		elif self._algo == 'AES256':
			key = urandom(32)
			iv = urandom(16)
			algo = algorithms.AES256(key)
			algo_name.append('aes256')
		elif self._algo == 'Camellia':
			key = urandom(32)
			iv = urandom(16)
			algo = algorithms.Camellia(key)
			algo_name.append('camellia')
		elif self._algo == 'ChaCha20':
			key = urandom(32)
			iv = urandom(12)  # ChaCha20 uses a 12-byte nonce
			algo = algorithms.ChaCha20(key, iv)
			algo_name.append('chacha20')

		need_padding = False
		if self._mode == 'CBC':
			need_padding = True
			mode = modes.CBC(iv)
			algo_name.append('cbc')
		elif self._mode == 'CFB':
			mode = modes.CFB(iv)
			algo_name.append('cfb')
		elif self._mode == 'CFB8':
			mode = modes.CFB8(iv)
			algo_name.append('cfb8')
		elif self._mode == 'CTR':
			mode = modes.CTR(iv)
			algo_name.append('ctr')
		elif self._mode == 'GCM':
			mode = modes.GCM(iv)
			algo_name.append('gcm')
		elif self._mode == 'OFB':
			mode = modes.OFB(iv)
			algo_name.append('ofb')

		if need_padding:
			if self._padding == 'PKCS7':
				padder = padding.PKCS7(algo.block_size).padder()
			elif self._padding == 'ANSIX923':
				padder = padding.ANSIX923(algo.block_size).padder()

		cipher = Cipher(algo, mode)
		encryptor = cipher.encryptor()

		if need_padding:
			data = padder.update(data) + padder.finalize()

		encrypted_data = encryptor.update(data) + encryptor.finalize()

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
						'issuer': x509.Name.build({'common_name': 'biscuit'}),
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
					'algorithm': '_'.join(algo_name),
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

		with open(private_key_path, 'wb') as f:
			if passphrase is None:
				encryption_algorithm = serialization.NoEncryption()
			else:
				encryption_algorithm = serialization.BestAvailableEncryption(passphrase.encode())

			private_key_bytes = private_key.private_bytes(
				encoding = serialization.Encoding.PEM,
				format = serialization.PrivateFormat.TraditionalOpenSSL,
				encryption_algorithm = encryption_algorithm
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
