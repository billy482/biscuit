#include <string>

typedef struct evp_pkey_st EVP_PKEY;

namespace Biscuit {
	class Key {
		public:
			Key(const std::string& filename);
			~Key();

			inline bool is_valid() const {
				return this->m_valid;
			}

		private:
			EVP_PKEY * load_keys(const std::string& filename);

			bool m_valid = false;
			EVP_PKEY * m_private_key = nullptr;
			EVP_PKEY * m_public_key = nullptr;
	};
}