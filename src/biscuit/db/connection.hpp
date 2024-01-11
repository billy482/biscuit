#ifndef __BISCUIT_DB_CONNECTION_HPP__
#define __BISCUIT_DB_CONNECTION_HPP__

#include "backup-id.hpp"
#include "block-id.hpp"
#include "file-id.hpp"
#include "host-id.hpp"
#include "key-id.hpp"
#include "sql-result.hpp"

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
				virtual ~Connection() = default;

				virtual bool connected() = 0;
				inline Driver& driver() {
					return this->m_driver;
				}
				inline const Driver& driver() const {
					return this->m_driver;
				}
				virtual BlockId get_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key) = 0;
				virtual bool has_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key) = 0;
				virtual FileId insert_file(const Source::FileInfo& file_info, const HostId& host) = 0;
				virtual bool is_newer_or_not_exists(const Source::FileInfo& file_info, const HostId& host_id) = 0;
				virtual BackupId start_backup() = 0;
				virtual HostId synchronize_host(const Host& host) = 0;
				virtual KeyId synchronize_key(const Key& key) = 0;

			protected:
				Connection(Driver& driver);

			private:
				Driver& m_driver;
		};
	}
}

#endif
