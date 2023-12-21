#include <spdlog/spdlog.h>
#include <sqlite3.h>

#include "connection.hpp"
#include "driver.hpp"
#include "../../host.hpp"
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

bool SqliteConnection::has_block(const QByteArray& digest, const QString& hash_algo, const Key& key) {
	const char * query = "SELECT id FROM block WHERE hash_algo = $1 AND hash = unhex($2) AND key = (SELECT id FROM keys WHERE fingerprint = $3) LIMIT 1";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_text(statement, 1, digest.data(), digest.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_text(statement, 2, hash_algo.toUtf8(), hash_algo.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	const QString fingerprint = key.fingerprint();
	ret = sqlite3_bind_text(statement, 3, fingerprint.toUtf8(), fingerprint.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	return ret == SQLITE_ROW;
}

bool SqliteConnection::is_newer_or_not_exists(const Source::FileInfo& file_info) {
	sqlite3_stmt * statement = nullptr;
	const char * query = "SELECT * FROM files WHERE path = $1 AND last_modified >= $2";
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	QByteArray path = file_info.path().toUtf8();
	ret = sqlite3_bind_text(statement, 1, path, path.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_int64(statement, 2, file_info.modified_time().currentSecsSinceEpoch());
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return ret == SQLITE_DONE;
}

void SqliteConnection::print_error() {
	auto logger = spdlog::get("database");
	logger->error("Sqlite: error: {}", sqlite3_errmsg(this->m_connection));
}

bool SqliteConnection::synchronize_host(const Host& host) {
	const char * query_select = "SELECT id FROM hosts WHERE hostname = $1 LIMIT 1";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query_select, strlen(query_select), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_text(statement, 1, host.hostname().toUtf8(), host.hostname().length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	sqlite3_finalize(statement);
	if (ret == SQLITE_ROW)
		return true;

	const char * query_insert = "INSERT INTO hosts(hostname) VALUES ($1)";
	ret = sqlite3_prepare(this->m_connection, query_insert, strlen(query_insert), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_text(statement, 1, host.hostname().toUtf8(), host.hostname().length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	sqlite3_finalize(statement);
	return ret != SQLITE_ERROR;
}

bool SqliteConnection::synchronize_key(const Key& key) {
	const char * query = "SELECT id FROM keys WHERE fingerprint = $1 LIMIT 1";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	const QString fingerprint = key.fingerprint();
	ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	sqlite3_finalize(statement);
	if (ret == SQLITE_ROW) {
		const char * query = "UPDATE keys SET last_use = unixepoch() WHERE fingerprint = $1";

		ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_step(statement);
		sqlite3_finalize(statement);
		return ret == SQLITE_DONE;
	} else {
		const char * query = "INSERT INTO keys(fingerprint, hash_algo, length) VALUES ($1, 'sha256', $2)";

		ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return false;
		}

		ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
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
