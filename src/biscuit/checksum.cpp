#include <openssl/evp.h>
#include <openssl/types.h>

#include "checksum.hpp"
#include "digest.hpp"

namespace Biscuit {
	static struct Checksum2Name {
		const char * name;
		ChecksumName value;
	} checksums[] = {
		{ "md5",    ChecksumName::MD5 },
		{ "sha1",   ChecksumName::SHA1 },
		{ "sha256", ChecksumName::SHA256 },
		{ "sha512", ChecksumName::SHA512 },

		{ nullptr, ChecksumName::Invalid }
	};
}

using namespace Biscuit;

Checksum::Checksum(ChecksumName name) : m_name(name) {
	const EVP_MD * md = nullptr;
	for (const Checksum2Name * ptr = checksums; ptr->name != nullptr; ptr++)
		if (name == ptr->value) {
			md = EVP_get_digestbyname(ptr->name);
			break;
		}

	if (md == nullptr)
		return;

	this->m_ctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex2(this->m_ctx, md, nullptr);
	this->m_valid = true;
}

Checksum::Checksum(const Block& block, ChecksumName name) : Checksum(name) {
	this->update(block);
}

Checksum::~Checksum() {
	if (this->m_ctx != nullptr)
		EVP_MD_CTX_free(this->m_ctx);
}


Digest Checksum::digest() {
	int size = EVP_MD_CTX_get_size(this->m_ctx);
	if (size <= 0 or size > 4096)
		return Digest();

	uint8_t * buffer = new uint8_t[size];
	uint32_t length = 0;
	EVP_DigestFinal(this->m_ctx, buffer, &length);

	Digest digest(buffer, length);
	delete [] buffer;
	return digest;
}

void Checksum::update(const Block& block) {
	this->update(block.buffer(), block.buffer_size());
}

void Checksum::update(const void * buffer, uint16_t buffer_size) {
	if (this->m_ctx != nullptr)
		EVP_DigestUpdate(this->m_ctx, buffer, buffer_size);
}