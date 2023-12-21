#ifndef __BISCUIT_DB_CONNECTION_HPP__
#define __BISCUIT_DB_CONNECTION_HPP__

class QByteArray;
class QString;

namespace Biscuit {
	class Host;
	class Key;

	namespace Source {
		class FileInfo;
	}

	namespace Db {
		class Driver;

		class Connection {
			public:
				virtual bool connected() = 0;
				inline Driver& driver() {
					return this->m_driver;
				}
				inline const Driver& driver() const {
					return this->m_driver;
				}
				virtual bool has_block(const QByteArray& digest, const QString& hash_algo, const Key& key) = 0;
				virtual bool is_newer_or_not_exists(const Source::FileInfo& file_info) = 0;
				virtual bool synchronize_host(const Host& host) = 0;
				virtual bool synchronize_key(const Key& key) = 0;

			protected:
				Connection(Driver& driver);

			private:
				Driver& m_driver;
		};
	}
}

#endif
