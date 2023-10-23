#ifndef __BISCUIT_KEY_HPP__
#define __BISCUIT_KEY_HPP__

class QByteArray;
class QFileInfo;
typedef struct evp_pkey_st EVP_PKEY;

namespace YAML {
	class Node;
}

namespace Biscuit {
	class Key {
		public:
			Key() = default;
			Key(const QFileInfo& filename);
			Key(Key&& key);
			~Key();

			static bool configure(const YAML::Node& config);
			QByteArray decrypt(const QByteArray& block) const;
			QByteArray encrypt(const QByteArray& block) const;
			inline bool is_valid() const {
				return this->m_valid;
			}

			Key& operator =(Key&& key);

		private:
			EVP_PKEY * load_keys(const QFileInfo& filename);

			bool m_valid = false;
			EVP_PKEY * m_private_key = nullptr;
			EVP_PKEY * m_public_key = nullptr;
			static Key ms_key;
	};
}

#endif