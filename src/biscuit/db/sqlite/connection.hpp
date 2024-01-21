#ifndef __BISCUIT_DB_SQLITE_CONNECTION_HPP__
#define __BISCUIT_DB_SQLITE_CONNECTION_HPP__

#include "query.hpp"
#include "../connection.hpp"

#include <QtCore/QHash>

typedef struct sqlite3 sqlite3;

namespace Biscuit {
	namespace Db {
		namespace Sqlite {
			class SqliteDriver;

			class SqliteConnection : public Connection {
				public:
					SqliteConnection(SqliteDriver& driver, sqlite3 * connection);
					virtual ~SqliteConnection();

					virtual bool connected();
					virtual BlockId get_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key);
					virtual bool has_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key);
					virtual FileId insert_file(const Source::FileInfo& file_info, const HostId& host);
					virtual bool is_newer_or_not_exists(const Source::FileInfo& file_info, const HostId& host_id);
					virtual BackupId start_backup();
					virtual HostId synchronize_host(const Host& host);
					virtual KeyId synchronize_key(const Key &key);

				private:
					sqlite3_stmt * prepare_query(const QString& query_name, const QString& query);
					void print_error();

					sqlite3 * m_connection;
					QHash<QString, SqliteQuery> m_prepared_statement;
			};
		}
	}
}

#endif
