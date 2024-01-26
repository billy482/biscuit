#include <algorithm>
#include <botan/data_src.h>
#include <botan/pkcs8.h>
#include <botan/pubkey.h>
#include <botan/x509_key.h>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "key.hpp"

using namespace Biscuit;

Key Key::ms_key;

Key::Key(const QFileInfo& filename) : m_filename(filename) {}

Key::Key(Key&& key) {
	this->m_private_key = std::move(key.m_private_key);
	this->m_public_key = std::move(key.m_public_key);

	key.m_private_key = nullptr;
	key.m_public_key = nullptr;
}


bool Key::configure(const YAML::Node& config) {
	const YAML::Node& path = config["path"];
	if (not path.IsScalar())
		return false;

	const std::string test = path.as<std::string>();

	auto logger = spdlog::get("core");
	logger->debug("Configure key: {}", test);

	QFileInfo private_key(QString(path.as<std::string>().c_str()));
	if (not private_key.exists())
		return false;

	Key::ms_key = Key(private_key);
	return true;
}

QByteArray Key::decrypt(const QByteArray& block) const {
	Botan::PK_Decryptor_EME dec(*this->m_private_key, this->m_rng, "OAEP(SHA-256)");
	Botan::secure_vector<uint8_t> decrypted = dec.decrypt(std::vector<uint8_t>(block.begin(), block.end()));
	return QByteArray(reinterpret_cast<const char *>(decrypted.data()), decrypted.size());
}

QByteArray Key::encrypt(const QByteArray& block) const {
	Botan::PK_Encryptor_EME enc(this->m_private_key ? *this->m_private_key : *this->m_public_key, this->m_rng, "OAEP(SHA-256)");
	const size_t block_size = enc.maximum_input_size();
	std::vector<uint8_t> encrypted;
	for (qsizetype position = 0; position < block.size(); position += block_size) {
		QByteArray::const_iterator iter_start = block.begin() + position;
		QByteArray::const_iterator iter_end = iter_start + std::min(block_size, static_cast<size_t>(block.size() - position));
		std::vector<uint8_t> encrypted_block = enc.encrypt(std::vector<uint8_t>(iter_start, iter_end), this->m_rng);
		encrypted.insert(encrypted.end(), encrypted_block.begin(), encrypted_block.end());
	}
	return QByteArray(reinterpret_cast<const char *>(encrypted.data()), encrypted.size());
}

QString Key::fingerprint() const {
	if (this->m_private_key)
		return QString(this->m_private_key->fingerprint_private("SHA-256").c_str());
	else if (this->m_public_key)
		return QString(this->m_public_key->fingerprint_public().c_str());
	else
		return QString();
}

Key& Key::get() {
	return Key::ms_key;
}

uint16_t Key::key_length() const {
	if (this->m_private_key)
		return this->m_private_key->key_length();
	else if (this->m_public_key)
		return this->m_public_key->key_length();
	else
		return 0;
}

bool Key::open_for_decrypt() {
	if (this->m_private_key)
		return true;

	Botan::DataSource_Stream private_file(this->m_filename.path().toLocal8Bit().data());
	this->m_private_key = Botan::PKCS8::load_key(private_file);

	return static_cast<bool>(this->m_private_key);
}

bool Key::open_for_encrypt() {
	if (this->m_private_key or this->m_public_key)
		return true;

	const QString filename = this->m_filename.filePath() + ".pub";
	Botan::DataSource_Stream public_file(filename.toLocal8Bit().data());
	this->m_public_key = std::unique_ptr<Botan::Public_Key>(Botan::X509::load_key(public_file));

	return static_cast<bool>(this->m_public_key);
}


Key& Key::operator=(Key&& key) {
	this->m_filename = key.m_filename;
	this->m_private_key = std::move(key.m_private_key);
	this->m_public_key = std::move(key.m_public_key);

	key.m_private_key = nullptr;
	key.m_public_key = nullptr;

	return *this;
}
