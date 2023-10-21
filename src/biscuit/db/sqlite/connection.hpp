#ifndef __BISCUIT_DB_SQLITE_CONNECTION_HPP__
#define __BISCUIT_DB_SQLITE_CONNECTION_HPP__

#include "../connection.hpp"

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
					virtual bool is_newer_or_exists(const Source::FileInfo& file_info);

				private:
					sqlite3 * m_connection;
			};
		}
	}
}

#endif