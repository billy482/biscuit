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
	this->m_prepared_statement.clear();
	this->m_connection = nullptr;
}


bool SqliteConnection::connected() {
	return true;
}

Biscuit::Db::BlockId SqliteConnection::get_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key) {
	sqlite3_stmt * statement = this->prepare_query("get_block", "SELECT id FROM blocks WHERE hash_algo = $1 AND hash = unhex($2) AND key = $3 LIMIT 1");
	if (statement == nullptr)
		return BlockId(SqlStatus::error);

	QByteArray raw_digest = hash_algo.toUtf8();
	int ret = sqlite3_bind_text(statement, 1, raw_digest.data(), raw_digest.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	QByteArray hex_data = digest.toHex();
	ret = sqlite3_bind_text(statement, 2, hex_data.data(), hex_data.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	bool ok;
	int key_id = key.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_bind_int(statement, 3, key_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	} else if (ret == SQLITE_OK or ret == SQLITE_DONE) {
		sqlite3_reset(statement);
		return BlockId(SqlStatus::not_found);
	} else if (ret == SQLITE_ROW) {
		int64_t block_id = sqlite3_column_int64(statement, 0);
		sqlite3_reset(statement);
		return BlockId(SqlStatus::has_result, QVariant(static_cast<qint64>(block_id)));
	} else {
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}
}

bool SqliteConnection::has_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key) {
	sqlite3_stmt * statement = this->prepare_query("get_block", "SELECT id FROM blocks WHERE hash_algo = $1 AND hash = unhex($2) AND key = $3 LIMIT 1");
	if (statement == nullptr)
		return false;

	int ret = sqlite3_bind_text(statement, 1, digest.data(), digest.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_bind_text(statement, 2, hash_algo.toUtf8(), hash_algo.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	bool ok;
	int key_id = key.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_bind_int(statement, 3, key_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	} else {
		sqlite3_reset(statement);
		return ret == SQLITE_ROW;
	}
}

Biscuit::Db::BlockId SqliteConnection::insert_block(const QByteArray& data, const QByteArray& digest, const QString& hash_algo, const KeyId& key) {
	sqlite3_stmt * statement = this->prepare_query("insert_block", "INSERT INTO blocks(hash_algo, hash, data, key) VALUES ($1, unhex($2), unhex($3), $4) RETURNING id");
	if (statement == nullptr)
		return BlockId(SqlStatus::error);

	QByteArray b_hash_algo = hash_algo.toUtf8();
	int ret = sqlite3_bind_text(statement, 1, b_hash_algo.data(), b_hash_algo.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	QByteArray hex_hash = digest.toHex();
	ret = sqlite3_bind_blob(statement, 2, hex_hash.data(), hex_hash.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	QByteArray hex_data = data.toHex();
	ret = sqlite3_bind_blob(statement, 3, hex_data.data(), hex_data.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	bool ok;
	int key_id = key.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_bind_int(statement, 4, key_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return BlockId(SqlStatus::error);
	} else if (ret == SQLITE_ROW) {
		int64_t block_id = sqlite3_column_int64(statement, 0);
		sqlite3_reset(statement);
		return BlockId(SqlStatus::has_result, QVariant(static_cast<qint64>(block_id)));
	} else {
		sqlite3_reset(statement);
		return BlockId(SqlStatus::not_found);
	}
}

Biscuit::Db::FileId SqliteConnection::insert_file(const Source::FileInfo& file_info, const HostId& host) {
	sqlite3_stmt * statement = this->prepare_query("insert_file", "INSERT INTO files(path, host) VALUES ($1, $2) RETURNING id");
	if (statement == nullptr)
		return FileId(SqlStatus::error);

	QByteArray path = file_info.path().toUtf8();
	int ret = sqlite3_bind_text(statement, 1, path.data(), path.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return FileId(SqlStatus::error);
	}

	bool ok;
	int host_id = host.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return FileId(SqlStatus::error);
	}
	ret = sqlite3_bind_int(statement, 2, host_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return FileId(SqlStatus::error);
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return FileId(SqlStatus::error);
	} else if (ret == SQLITE_ROW) {
		int64_t file_id = sqlite3_column_int64(statement, 0);
		sqlite3_reset(statement);
		return FileId(SqlStatus::has_result, QVariant(static_cast<qint64>(file_id)));
	} else {
		sqlite3_reset(statement);
		return FileId(SqlStatus::not_found);
	}
}

bool SqliteConnection::is_newer_or_not_exists(const Source::FileInfo& file_info, const HostId& host) {
	sqlite3_stmt * statement = this->prepare_query("is_newer_or_not_exists", "SELECT * FROM files WHERE path = $1 AND last_modified >= $2 AND host = $3");
	if (statement == nullptr)
		return false;

	QByteArray path = file_info.path().toUtf8();
	int ret = sqlite3_bind_text(statement, 1, path.data(), path.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_bind_int64(statement, 2, file_info.modified_time().currentSecsSinceEpoch());
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	bool ok;
	int host_id = host.value().toInt(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}
	ret = sqlite3_bind_int(statement, 3, host_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR)
		this->print_error();
	sqlite3_reset(statement);
	return ret == SQLITE_DONE;
}

bool SqliteConnection::link_file_to_block(const FileId& file_id, const BlockId& block_id, quint32 sequence) {
	sqlite3_stmt * statement = this->prepare_query("link_file_to_block", "INSERT INTO files2blocks VALUES ($1, $2, $3)");
	if (statement == nullptr)
		return false;

	bool ok;
	int64_t int_file_id = file_id.value().toLongLong(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	int ret = sqlite3_bind_int64(statement, 1, int_file_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	int64_t int_block_id = block_id.value().toLongLong(&ok);
	if (not ok) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_bind_int64(statement, 2, int_block_id);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_bind_int(statement, 3, sequence);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return false;
	}

	sqlite3_reset(statement);
	return true;
}

sqlite3_stmt * SqliteConnection::prepare_query(const QString& query_name, const QString& query) {
	QHash<QString, SqliteQuery>::iterator iter = this->m_prepared_statement.find(query_name);
	if (iter == this->m_prepared_statement.end()) {
		SqliteQuery sqlite_query(query, this->m_connection);
		if (sqlite_query.has_error()) {
			this->print_error();
			return nullptr;
		} else {
			this->m_prepared_statement.insert(query_name, sqlite_query);
			return this->prepare_query(query_name, query);
		}
	} else {
		SqliteQuery& sqlite_query = iter.value();
		return sqlite_query.statement();
	}
}

void SqliteConnection::print_error() {
	auto logger = spdlog::get("database");
	logger->error("Sqlite: error: {}", sqlite3_errmsg(this->m_connection));
}

Biscuit::Db::BackupId SqliteConnection::start_backup() {
	sqlite3_stmt * statement = this->prepare_query("get_last_backup", "SELECT COALESCE(MAX(id), -1) FROM backups WHERE end_time IS NOT NULL");
	if (statement == nullptr)
		return BackupId();

	int ret = sqlite3_step(statement);
	int32_t backup_id = sqlite3_column_int(statement, 0);

	statement = this->prepare_query("start_backup", "INSERT INTO backups (parent_backup) VALUES ($1) RETURNING id");
	if (statement == nullptr)
		return BackupId();

	if (backup_id != -1)
		ret = sqlite3_bind_int(statement, 0, backup_id);
	else
		ret = sqlite3_bind_null(statement, 0);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		return BackupId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		return BackupId();
	}

	backup_id = sqlite3_column_int(statement, 0);
	sqlite3_reset(statement);
	return BackupId(SqlStatus::has_result, QVariant(backup_id));
}

Biscuit::Db::HostId SqliteConnection::synchronize_host(const Host& host) {
	sqlite3_stmt * statement = this->prepare_query("get_host_by_id", "SELECT id FROM hosts WHERE hostname = $1 LIMIT 1");
	if (statement == nullptr)
		return HostId();

	QByteArray hostname = host.hostname().toUtf8();
	int ret = sqlite3_bind_text(statement, 1, hostname.data(), hostname.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return HostId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ROW) {
		int32_t host_id = sqlite3_column_int(statement, 0);
		sqlite3_reset(statement);
		return HostId(SqlStatus::has_result, QVariant(host_id));
	}

	sqlite3_reset(statement);


	statement = this->prepare_query("insert_host", "INSERT INTO hosts(hostname) VALUES ($1) RETURNING id");
	if (statement == nullptr)
		return HostId();

	ret = sqlite3_bind_text(statement, 1, hostname.data(), hostname.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return HostId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return HostId();
	}

	int32_t host_id = sqlite3_column_int(statement, 0);
	sqlite3_reset(statement);
	return HostId(SqlStatus::has_result, QVariant(host_id));
}

Biscuit::Db::KeyId SqliteConnection::synchronize_key(const Key& key) {
	sqlite3_stmt * statement = this->prepare_query("get_key_by_fingerprint", "SELECT id FROM keys WHERE fingerprint = $1 LIMIT 1");
	if (statement == nullptr)
		return KeyId();

	const QByteArray fingerprint = key.fingerprint().toUtf8();
	int ret = sqlite3_bind_text(statement, 1, fingerprint.data(), fingerprint.length(), nullptr);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return KeyId();
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ERROR) {
		this->print_error();
		sqlite3_reset(statement);
		return KeyId();
	}

	if (ret == SQLITE_ROW) {
		int key_id = sqlite3_column_int(statement, 0);
		sqlite3_reset(statement);

		statement = this->prepare_query("update_host", "UPDATE keys SET last_use = unixepoch() WHERE id = $1");
		if (statement == nullptr)
			return KeyId();

		ret = sqlite3_bind_int(statement, 1, key_id);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_reset(statement);
			return KeyId();
		}

		ret = sqlite3_step(statement);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_reset(statement);
			return KeyId();
		}

		sqlite3_reset(statement);
		return KeyId(SqlStatus::has_result, QVariant(key_id));
	} else {
		sqlite3_reset(statement);
		statement = this->prepare_query("insert_host", "INSERT INTO keys(fingerprint, hash_algo, length) VALUES ($1, 'sha256', $2) RETURNING id");
		if (statement == nullptr)
			return KeyId();

		ret = sqlite3_bind_text(statement, 1, fingerprint.data(), fingerprint.length(), nullptr);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_reset(statement);
			return KeyId();
		}

		ret = sqlite3_bind_int(statement, 2, key.key_length());
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_reset(statement);
			return KeyId();
		}

		ret = sqlite3_step(statement);
		if (ret == SQLITE_ERROR) {
			this->print_error();
			sqlite3_reset(statement);
			return KeyId();
		}

		int key_id = sqlite3_column_int(statement, 0);
		sqlite3_reset(statement);
		return KeyId(SqlStatus::has_result, QVariant(key_id));
	}
}
