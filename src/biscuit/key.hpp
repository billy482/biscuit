#ifndef __BISCUIT_KEY_HPP__
#define __BISCUIT_KEY_HPP__

#include <botan/auto_rng.h>
#include <botan/pk_keys.h>
#include <cstdint>
#include <memory>
#include <QtCore/QFileInfo>

class QByteArray;

namespace YAML {
	class Node;
}

namespace Biscuit {
	class Key {
		public:
			Key() = default;
			Key(const QFileInfo& filename);
			Key(Key&& key);
			~Key() = default;

			static bool configure(const YAML::Node& config);
			QByteArray decrypt(const QByteArray& block) const;
			QByteArray encrypt(const QByteArray& block) const;
			QString fingerprint() const;
			static Key& get();
			uint16_t key_length() const;
			bool open_for_decrypt();
			bool open_for_encrypt();

			Key& operator =(Key&& key);

		private:
			QFileInfo m_filename;
			mutable Botan::AutoSeeded_RNG m_rng;
			std::unique_ptr<Botan::Private_Key> m_private_key = nullptr;
			std::unique_ptr<Botan::Public_Key> m_public_key = nullptr;
			static Key ms_key;
	};
}

#endif
