#include <openssl/store.h>
#include <openssl/ui.h>

#include "key.hpp"

using namespace Biscuit;
using std::string;

Key::Key(const string& filename) {
	this->m_private_key = this->load_keys(filename);
	this->m_public_key = this->load_keys(filename + ".pub");
	this->m_valid = this->m_private_key != nullptr and this->m_public_key != nullptr;
}

Key::~Key() {
	if (this->m_private_key != nullptr)
		EVP_PKEY_free(this->m_private_key);
	if (this->m_public_key != nullptr)
		EVP_PKEY_free(this->m_public_key);
}


EVP_PKEY * Key::load_keys(const string& filename) {
	OSSL_STORE_CTX * ctx = OSSL_STORE_open_ex(filename.c_str(), nullptr, nullptr, UI_OpenSSL(), nullptr, nullptr, nullptr, nullptr);

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
	}

	OSSL_STORE_close(ctx);

	return key;
}