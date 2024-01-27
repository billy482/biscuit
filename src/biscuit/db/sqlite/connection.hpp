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

#ifndef __BISCUIT_DB_SQLITE_CONNECTION_HPP__
#define __BISCUIT_DB_SQLITE_CONNECTION_HPP__

#include "query.hpp"
#include "../connection.hpp"

#include <QtCore/QHash>

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
					virtual bool finish_backup(const BackupId& backup_id);
					virtual BlockId get_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key);
					virtual FileId get_file(const Source::FileInfo& file_info, const HostId& host);
					virtual MetadataId get_metadata(const QByteArray& digest, const QString& hash_algo);
					virtual bool has_block(const QByteArray& digest, const QString& hash_algo, const KeyId& key);
					virtual BlockId insert_block(const QByteArray& block, const QByteArray& digest, const QString& hash_algo, const KeyId& key);
					virtual FileId insert_file(const Source::FileInfo& file_info, const HostId& host);
					virtual MetadataId insert_metadata(const QByteArray& data, const QByteArray& digest, const QString& hash_algo);
					virtual bool is_newer_or_not_exists(const Source::FileInfo& file_info, const HostId& host_id);
					virtual bool link_file_to_backup(const FileId& file_id, const BackupId& backup_id, const MetadataId& metadata_id);
					virtual bool link_file_to_block(const FileId& file_id, const BlockId& block_id, quint32 sequence);
					virtual BackupId start_backup();
					virtual HostId synchronize_host(const Host& host);
					virtual KeyId synchronize_key(const Key &key);

				private:
					sqlite3_stmt * prepare_query(const QString& query_name, const QString& query);
					void print_error();

					sqlite3 * m_connection;
					QHash<QString, SqliteQuery> m_prepared_statement;
			};
		}
	}
}

#endif
