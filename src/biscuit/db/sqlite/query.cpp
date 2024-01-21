#include <sqlite3.h>

#include "query.hpp"

using namespace Biscuit::Db::Sqlite;

SqliteQuery::SqliteQuery(const QString& query, sqlite3 * connection) : m_connection(connection), m_query(query.toUtf8()) {
	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(connection, this->m_query.data(), this->m_query.length(), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		this->m_has_error = true;
	} else
		this->m_statement = statement;
}

SqliteQuery::SqliteQuery(const SqliteQuery& query) : m_connection(query.m_connection), m_query(query.m_query) {
	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, this->m_query.data(), this->m_query.length(), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		this->m_has_error = true;
	} else
		this->m_statement = statement;
}

SqliteQuery::SqliteQuery(SqliteQuery&& query) : m_connection(query.m_connection), m_query(std::move(query.m_query)), m_statement(query.m_statement), m_has_error(query.m_has_error) {
	query.m_statement = nullptr;
}

SqliteQuery::~SqliteQuery() {
	if (this->m_statement != nullptr)
		sqlite3_finalize(this->m_statement);
}


SqliteQuery& SqliteQuery::operator=(const SqliteQuery& query) {
	this->m_connection = query.m_connection;
	this->m_query = query.m_query;

	if (this->m_statement != nullptr)
		sqlite3_finalize(this->m_statement);

	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, this->m_query.data(), this->m_query.length(), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		this->m_has_error = true;
	} else {
		this->m_statement = statement;
		this->m_has_error = false;
	}

	return *this;
}

SqliteQuery& SqliteQuery::operator=(SqliteQuery&& query) {
	this->m_connection = query.m_connection;
	this->m_query = std::move(this->m_query);
	this->m_has_error = query.m_has_error;

	if (this->m_statement != nullptr)
		sqlite3_finalize(this->m_statement);
	this->m_statement = query.m_statement;
	query.m_statement = nullptr;

	return *this;
}
