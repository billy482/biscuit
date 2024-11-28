/***************************************************************************\
*                         __    _                 _ __                      *
*                        / /_  (_)___________  __(_) /_                     *
*                       / __ \/ / ___/ ___/ / / / / __/                     *
*                      / /_/ / (__  ) /__/ /_/ / / /_                       *
*                     /_.___/_/____/\___/\__,_/_/\__/                       *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of biscuit                                           *
*                                                                           *
*  biscuit is free software; you can redistribute it and/or                 *
*  modify it under the terms of the GNU General Public License              *
*  as published by the Free Software Foundation; either version 3           *
*  of the License, or (at your option) any later version.                   *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program; if not, write to the Free Software              *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                       *
*  Boston, MA  02110-1301, USA.                                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  Copyright (C) 2024, Guillaume Clercin <guillaume.clercin@billy482.net>   *
\***************************************************************************/

#include <sqlite3.h>

#include "query.hpp"

using namespace Biscuit::Db::Sqlite;

SqliteQuery::SqliteQuery(const String& query, sqlite3 * connection) : m_connection(connection), m_query(query) {
	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(connection, this->m_query, this->m_query.utf8_length(), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		this->m_has_error = true;
	} else
		this->m_statement = statement;
}

SqliteQuery::SqliteQuery(const SqliteQuery& query) : m_connection(query.m_connection), m_query(query.m_query) {
	sqlite3_stmt * statement = nullptr;
	int ret = sqlite3_prepare(this->m_connection, this->m_query, this->m_query.utf8_length(), &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);
		this->m_has_error = true;
	} else
		this->m_statement = statement;
}

SqliteQuery::SqliteQuery(SqliteQuery&& query) : m_connection(query.m_connection), m_query(query.m_query), m_statement(query.m_statement), m_has_error(query.m_has_error) {
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
	int ret = sqlite3_prepare(this->m_connection, this->m_query, this->m_query.utf8_length(), &statement, nullptr);
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
	this->m_query = this->m_query;
	this->m_has_error = query.m_has_error;

	if (this->m_statement != nullptr)
		sqlite3_finalize(this->m_statement);
	this->m_statement = query.m_statement;
	query.m_statement = nullptr;

	return *this;
}
