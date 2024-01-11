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

Biscuit::Db::BlockId SqliteConnection::get_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key) {
	sqlite3_stmt * statement = nullptr;
	const char * query = "SELECT id FROM blocks WHERE hash_algo = $1 AND hash = unhex($2) AND key = $3 LIMIT 1";
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	}

	QByteArray raw_digest = hash_algo.toUtf8();
	ret = sqlite3_bind_text(statement, 1, raw_digest.data(), raw_digest.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_bind_text(statement, 2, digest.data(), digest.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	}

	bool ok;
	int key_id = key.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_bind_int(statement, 3, key_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	} else if (ret == SQLITE_OK) {
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::not_found);
	} else if (ret == SQLITE_ROW) {
		int64_t block_id = sqlite3_column_int64(statement, 0);
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::has_result, QVariant(static_cast<qint64>(block_id)));
	} else {
		sqlite3_finalize(statement);
		return BlockId(SqlStatus::error);
	}
}

bool SqliteConnection::has_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key) {
	const char * query = "SELECT id FROM block WHERE hash_algo = $1 AND hash = unhex($2) AND key = $3 LIMIT 1";

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

	bool ok;
	int key_id = key.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_bind_int(statement, 3, key_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	} else
		return ret == SQLITE_ROW;
}

Biscuit::Db::FileId SqliteConnection::insert_file(const Source::FileInfo& file_info, const HostId& host) {
	sqlite3_stmt * statement = nullptr;
	const char * query = "INSERT INTO files(path, host) VALUES ($1, $2) RETURNING id";
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return FileId(SqlStatus::error);
	}

	QByteArray path = file_info.path().toUtf8();
	ret = sqlite3_bind_text(statement, 1, path, path.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return FileId(SqlStatus::error);
	}

	bool ok;
	int host_id = host.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_finalize(statement);
		return FileId(SqlStatus::error);
	}
	ret = sqlite3_bind_int(statement, 2, host_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return FileId(SqlStatus::error);
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return FileId(SqlStatus::error);
	}

	int64_t file_id = sqlite3_column_int64(statement, 0);
	sqlite3_finalize(statement);

	return FileId(SqlStatus::has_result, QVariant(static_cast<qint64>(file_id)));
}

bool SqliteConnection::is_newer_or_not_exists(const Source::FileInfo& file_info, const HostId& host) {
	sqlite3_stmt * statement = nullptr;
	const char * query = "SELECT * FROM files WHERE path = $1 AND last_modified >= $2 AND host = $3";
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

	bool ok;
	int host_id = host.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_finalize(statement);
		return false;
	}
	ret = sqlite3_bind_int(statement, 3, host_id);
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

Biscuit::Db::BackupId SqliteConnection::start_backup() {
	const char * query_select = "SELECT COALESCE(MAX(id), -1) FROM backups WHERE end_time IS NOT NULL";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query_select, strlen(query_select), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BackupId();
	}

	ret = sqlite3_step(statement);
	int32_t backup_id = sqlite3_column_int(statement, 0);
	sqlite3_finalize(statement);

	const char * query_insert = "INSERT INTO backups (parent_backup) VALUES ($1) RETURNING id";
	ret = sqlite3_prepare(this->m_connection, query_insert, strlen(query_insert), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BackupId();
	}

	if (backup_id != -1)
		ret = sqlite3_bind_int(statement, 0, backup_id);
	else
		ret = sqlite3_bind_null(statement, 0);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BackupId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return BackupId();
	}

	backup_id = sqlite3_column_int(statement, 0);
	sqlite3_finalize(statement);
	return BackupId(SqlStatus::has_result, QVariant(backup_id));
}

Biscuit::Db::HostId SqliteConnection::synchronize_host(const Host& host) {
	const char * query_select = "SELECT id FROM hosts WHERE hostname = $1 LIMIT 1";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query_select, strlen(query_select), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return HostId();
	}

	ret = sqlite3_bind_text(statement, 1, host.hostname().toUtf8(), host.hostname().length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return HostId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ROW) {
		int32_t host_id = sqlite3_column_int(statement, 0);
		sqlite3_finalize(statement);
		return HostId(SqlStatus::has_result, QVariant(host_id));
	} else
		sqlite3_finalize(statement);

	const char * query_insert = "INSERT INTO hosts(hostname) VALUES ($1) RETURNING id";
	ret = sqlite3_prepare(this->m_connection, query_insert, strlen(query_insert), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return HostId();
	}

	ret = sqlite3_bind_text(statement, 1, host.hostname().toUtf8(), host.hostname().length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return HostId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return HostId();
	}

	int32_t host_id = sqlite3_column_int(statement, 0);
	sqlite3_finalize(statement);
	return HostId(SqlStatus::has_result, QVariant(host_id));
}

Biscuit::Db::KeyId SqliteConnection::synchronize_key(const Key& key) {
	const char * query = "SELECT id FROM keys WHERE fingerprint = $1 LIMIT 1";

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return KeyId();
	}

	const QString fingerprint = key.fingerprint();
	ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return KeyId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_finalize(statement);
		return KeyId();
	}

	if (ret == SQLITE_ROW) {
		int key_id = sqlite3_column_int(statement, 0);
		sqlite3_finalize(statement);

		const char * query = "UPDATE keys SET last_use = unixepoch() WHERE id = $1";

		ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		ret = sqlite3_bind_int(statement, 1, key_id);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		ret = sqlite3_step(statement);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		sqlite3_finalize(statement);
		return KeyId(SqlStatus::has_result, QVariant(key_id));
	} else {
		sqlite3_finalize(statement);

		const char * query = "INSERT INTO keys(fingerprint, hash_algo, length) VALUES ($1, 'sha256', $2) RETURNING id";

		ret = sqlite3_prepare(this->m_connection, query, strlen(query), &statement, nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		ret = sqlite3_bind_text(statement, 1, fingerprint.toUtf8(), fingerprint.length(), nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		ret = sqlite3_bind_int(statement, 2, key.key_length());
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		ret = sqlite3_step(statement);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_finalize(statement);
			return KeyId();
		}

		int key_id = sqlite3_column_int(statement, 0);
		sqlite3_finalize(statement);
		return KeyId(SqlStatus::has_result, QVariant(key_id));
	}
}
