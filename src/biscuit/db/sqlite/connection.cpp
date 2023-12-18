#include <sqlite3.h>

#include "connection.hpp"
#include "driver.hpp"
#include "../../key.hpp"
#include "../../source/file-info.hpp"

using namespace Biscuit::Db::Sqlite;

SqliteConnection::SqliteConnection(SqliteDriver& driver, sqlite3 * connection) : Connection(driver), m_connection(connection) {}

SqliteConnection::~SqliteConnection() {
	sqlite3_close_v2(this->m_connection);
	this->m_connection = nullptr;
}


bool SqliteConnection::connected() {
	return true;
}

bool SqliteConnection::is_newer_or_not_exists(const Source::FileInfo& file_info) {
	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, "SELECT * FROM files WHERE path = $1 AND last_modified >= $2", 59, &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		return false;
	}

	QByteArray path = file_info.path().toUtf8();
	ret = sqlite3_bind_text(statement, 1, path, path.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_int64(statement, 2, file_info.modified_time().currentSecsSinceEpoch());
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return ret == SQLITE_DONE;
}

bool SqliteConnection::synchronize_key(const Key& key) {
	const char * query = "SELECT id FROM keys WHERE fingerprint = $1 LIMIT 1";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		return false;
	}

	const QString fingerprint = key.fingerprint();
	ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	sqlite3_finalize(statement);
	if (ret == SQLITE_ROW) {
		const char * query = "UPDATE keys SET last_use = NOW() WHERE fingerprint = $1";

		ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
		if (ret == SQLITE_ERROR) {
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
		if (ret == SQLITE_ERROR) {
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_step(statement);
		sqlite3_finalize(statement);
		return ret == SQLITE_DONE;
	} else {
		const char * query = "INSERT INTO key VALUES (DEFAULT, $1, 'sha256', $2, DEFAULT, DEFAULT)";

		ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
		if (ret == SQLITE_ERROR) {
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
		if (ret == SQLITE_ERROR) {
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_bind_int(statement, 2, key.key_length());

		ret = sqlite3_step(statement);
		sqlite3_finalize(statement);
		return ret == SQLITE_DONE;
	}

	return ret == SQLITE_DONE;
}
