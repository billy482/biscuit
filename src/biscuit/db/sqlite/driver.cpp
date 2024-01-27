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

#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <QtCore/QMutex>
#include <yaml-cpp/yaml.h>

#include "connection.hpp"
#include "driver.hpp"

using namespace Biscuit::Db::Sqlite;
using Biscuit::Db::Connection;
using YAML::Node;

SqliteDriver::SqliteDriver(const QFileInfo& path) : Driver("sqlite"), m_path(path) {
	auto logger = spdlog::get("database");
	logger->info("Using Sqlite database (version: {})", sqlite3_libversion());

	sqlite3 * connection = nullptr;
	QByteArray filename = this->m_path.absoluteFilePath().toUtf8();
	logger->debug("Opening database {}", filename.data());
	int ret = sqlite3_open_v2(filename.data(), &connection, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
	if (ret != 0) {
		logger->error("Error while opening database {} because {}", filename.data(), sqlite3_errmsg(connection));
		sqlite3_close_v2(connection);
	}

	sqlite3_stmt * statement = nullptr;
	ret = sqlite3_prepare(connection, "SELECT * FROM configuration", 27, &statement, nullptr);
	if (ret == SQLITE_ERROR) {
		sqlite3_finalize(statement);

		logger->info("Creating new database");
		if (this->create_db(connection))
			logger->info("Database created");
		else {
			logger->error("Error while creating database");
			sqlite3_close_v2(connection);
		}
	}

	this->m_connection = connection;
}


bool SqliteDriver::close() {
	if (this->m_connection != nullptr)
		sqlite3_close_v2(this->m_connection);
	this->m_connection = nullptr;
	return true;
}

SqliteDriver * SqliteDriver::configure(const Node& node) {
	const Node& path = node["path"];
	if (not path.IsScalar())
		return nullptr;

	return new SqliteDriver(QFileInfo(path.as<std::string>().c_str()));
}

bool SqliteDriver::create_db(sqlite3 * connection) {
	auto logger = spdlog::get("database");

	const char * queries[] = {
		"CREATE TABLE keys (id INTEGER PRIMARY KEY AUTOINCREMENT, fingerprint BLOB NOT NULL UNIQUE, hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')), length INTEGER NOT NULL CHECK (length > 0), first_use INTEGER NOT NULL DEFAULT (unixepoch()), last_use INTEGER NOT NULL DEFAULT (unixepoch()))",
		"CREATE TABLE blocks (id INTEGER PRIMARY KEY AUTOINCREMENT, hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')), hash BLOB NOT NULL, data BLOB NOT NULL, key INTEGER REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT)",
		"CREATE TABLE hosts (id INTEGER PRIMARY KEY AUTOINCREMENT, hostname TEXT NOT NULL)",
		"CREATE TABLE files (id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT NOT NULL, last_modified INTEGER NOT NULL DEFAULT (unixepoch()), host INTEGER NULL REFERENCES host(id) ON UPDATE CASCADE ON DELETE RESTRICT)",
		"CREATE TABLE files2blocks (file INTEGER NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE, block INTEGER NOT NULL REFERENCES blocks(id) ON UPDATE CASCADE ON DELETE CASCADE, sequence INTEGER NOT NULL CHECK (sequence >= 0))",
		"CREATE TABLE backups (id INTEGER PRIMARY KEY AUTOINCREMENT, start_time INTEGER NOT NULL DEFAULT (unixepoch()), end_time INTEGER, size INTEGER CHECK (size >= 0), increment_size INTEGER CHECK (increment_size >= 0), parent_backup INTEGER REFERENCES backups(id) ON UPDATE CASCADE ON DELETE SET NULL)",
		"CREATE TABLE metadata (id INTEGER PRIMARY KEY AUTOINCREMENT, hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')), hash BLOB NOT NULL, data BLOB NOT NULL)",
		"CREATE TABLE backups2files (backup INTEGER NOT NULL REFERENCES backups(id) ON UPDATE CASCADE ON DELETE CASCADE, file INTEGER NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE, metadata INTEGER NOT NULL REFERENCES metadata(id) ON UPDATE CASCADE ON DELETE CASCADE)",
		"CREATE TABLE configuration (key TEXT PRIMARY KEY, value TEXT NULL)",
		"INSERT INTO configuration VALUES ('db_version', '1')",
		nullptr
	};

	for (uint8_t i = 0; queries[i] != nullptr; i++) {
		sqlite3_stmt * statement = nullptr;
		int ret = sqlite3_prepare_v2(connection, queries[i], strlen(queries[i]), &statement, nullptr);
		if (ret != SQLITE_OK) {
			logger->critical("Sqlite: error while preparing query: {} because {}", queries[i], sqlite3_errmsg(connection));
			return false;
		}

		ret = sqlite3_step(statement);
		sqlite3_finalize(statement);

		if (ret != SQLITE_DONE)
			return false;
	}

	return true;
}

Connection * SqliteDriver::open() {
	if (this->m_connection != nullptr)
		return new SqliteConnection(*this, this->m_connection);
	else
		return nullptr;
}
