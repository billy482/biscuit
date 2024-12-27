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

#ifndef __BISCUIT_DB_SQLITE_QUERY_HPP__
#define __BISCUIT_DB_SQLITE_QUERY_HPP__

#include "../../string.hpp"

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

namespace Biscuit {
	namespace Db {
		namespace Sqlite {
			class SqliteQuery {
				public:
					SqliteQuery(const String& query, sqlite3 * connection);
					SqliteQuery(const SqliteQuery& query);
					SqliteQuery(SqliteQuery&& query);
					~SqliteQuery();

					inline bool has_error() const {
						return this->m_has_error;
					}
					inline String& query() {
						return this->m_query;
					}
					inline const String& query() const {
						return this->m_query;
					}
					inline sqlite3_stmt * statement() {
						return this->m_statement;
					}

					SqliteQuery& operator=(const SqliteQuery& query);
					SqliteQuery& operator=(SqliteQuery&& query);

				private:
					sqlite3 * m_connection;
					String m_query;
					sqlite3_stmt * m_statement = nullptr;
					bool m_has_error = false;
			};
		}
	}
}

#endif
