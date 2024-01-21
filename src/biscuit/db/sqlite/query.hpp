#ifndef __BISCUIT_DB_SQLITE_QUERY_HPP__
#define __BISCUIT_DB_SQLITE_QUERY_HPP__

#include <QtCore/QString>

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

namespace Biscuit {
	namespace Db {
		namespace Sqlite {
			class SqliteQuery {
				public:
					SqliteQuery(const QString& query, sqlite3 * connection);
					SqliteQuery(const SqliteQuery& query);
					SqliteQuery(SqliteQuery&& query);
					~SqliteQuery();

					inline bool has_error() const {
						return this->m_has_error;
					}
					inline QByteArray& query() {
						return this->m_query;
					}
					inline const QByteArray& query() const {
						return this->m_query;
					}
					inline sqlite3_stmt * statement() {
						return this->m_statement;
					}

					SqliteQuery& operator=(const SqliteQuery& query);
					SqliteQuery& operator=(SqliteQuery&& query);

				private:
					sqlite3 * m_connection;
					QByteArray m_query;
					sqlite3_stmt * m_statement = nullptr;
					bool m_has_error = false;
			};
		}
	}
}

#endif
