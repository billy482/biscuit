#include <openssl/store.h>
#include <openssl/ui.h>
#include <QtCore/QByteArray>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <yaml-cpp/yaml.h>

#include "key.hpp"

using namespace Biscuit;

Key Key::ms_key;

Key::Key(const QFileInfo& filename) {
	this->m_private_key = this->load_keys(filename);
	this->m_public_key = this->load_keys(QFileInfo(filename.absoluteFilePath() + QString(".pub")));
	this->m_valid = this->m_private_key != nullptr and this->m_public_key != nullptr;
}

Key::Key(Key&& key) {
	this->m_private_key = key.m_private_key;
	this->m_public_key = key.m_public_key;
	this->m_valid = key.m_valid;

	key.m_private_key = nullptr;
	key.m_public_key = nullptr;
	key.m_valid = false;
}

Key::~Key() {
	if (this->m_private_key != nullptr)
		EVP_PKEY_free(this->m_private_key);
	if (this->m_public_key != nullptr)
		EVP_PKEY_free(this->m_public_key);
}


bool Key::configure(const YAML::Node& config) {
	const YAML::Node& path = config["path"];
	if (not path.IsScalar()) {
		return false;
	}

	Key::ms_key = Key(QFileInfo(QString(path.as<std::string>().c_str())));
	return Key::ms_key.is_valid();
}

QByteArray Key::decrypt(const QByteArray& block) const {
	EVP_PKEY_CTX * ctx = EVP_PKEY_CTX_new(this->m_private_key, nullptr);
	if (EVP_PKEY_decrypt_init(ctx) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}
	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	size_t length = 0;
	if (EVP_PKEY_decrypt(ctx, nullptr, &length, reinterpret_cast<const unsigned char *>(block.data()), block.length()) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	uint8_t * buffer = new uint8_t[length];
	int result = EVP_PKEY_decrypt(ctx, buffer, &length, reinterpret_cast<const unsigned char *>(block.data()), block.length());

	EVP_PKEY_CTX_free(ctx);

	if (result <= 0) {
		delete[] buffer;
		return QByteArray();
	} else {
		QByteArray result(reinterpret_cast<const char *>(buffer), length);
		delete[] buffer;
		return result;
	}
}

QByteArray Key::encrypt(const QByteArray& block) const {
	EVP_PKEY_CTX * ctx = EVP_PKEY_CTX_new(this->m_public_key, nullptr);
	if (EVP_PKEY_encrypt_init(ctx) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}
	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	size_t length = 0;
	if (EVP_PKEY_encrypt(ctx, nullptr, &length, reinterpret_cast<const unsigned char *>(block.data()), block.length()) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	uint8_t * buffer = new uint8_t[length];
	int result = EVP_PKEY_encrypt(ctx, buffer, &length, reinterpret_cast<const unsigned char *>(block.data()), block.length());

	EVP_PKEY_CTX_free(ctx);

	if (result <= 0) {
		delete[] buffer;
		return QByteArray();
	} else {
		QByteArray result(reinterpret_cast<const char *>(buffer), length);
		delete[] buffer;
		return result;
	}
}

EVP_PKEY * Key::load_keys(const QFileInfo& filename) {
	OSSL_STORE_CTX * ctx = OSSL_STORE_open_ex(filename.absoluteFilePath().toUtf8().data(), nullptr, nullptr, UI_OpenSSL(), nullptr, nullptr, nullptr, nullptr);
	if (ctx == nullptr)
		return nullptr;

	EVP_PKEY * key = nullptr;
	while (key == nullptr and !OSSL_STORE_eof(ctx)) {
		OSSL_STORE_INFO * info = OSSL_STORE_load(ctx);
		if (info == nullptr)
			continue;

		int type = OSSL_STORE_INFO_get_type(info);
		switch (type) {
			case OSSL_STORE_INFO_PKEY:
				key = OSSL_STORE_INFO_get1_PKEY(info);
				break;

			case OSSL_STORE_INFO_PUBKEY:
				key = OSSL_STORE_INFO_get1_PUBKEY(info);
				break;
		}

		OSSL_STORE_INFO_free(info);
	}

	OSSL_STORE_close(ctx);

	return key;
}


Key& Key::operator=(Key&& key) {
	this->m_private_key = key.m_private_key;
	this->m_public_key = key.m_public_key;
	this->m_valid = key.m_valid;

	key.m_private_key = nullptr;
	key.m_public_key = nullptr;
	key.m_valid = false;

	return *this;
}