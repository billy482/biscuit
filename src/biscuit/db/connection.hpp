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

#ifndef __BISCUIT_DB_CONNECTION_HPP__
#define __BISCUIT_DB_CONNECTION_HPP__

#include <cstdint>
#include <vector>

#include "backup-id.hpp"
#include "block-id.hpp"
#include "file-id.hpp"
#include "host-id.hpp"
#include "key-id.hpp"
#include "metadata-id.hpp"
#include "sql-result.hpp"

namespace Biscuit {
	class Host;
	class Key;
	class String;

	namespace Source {
		class FileInfo;
	}

	namespace Db {
		class Driver;

		class Connection {
			public:
				virtual ~Connection() = default;

				virtual bool connected() = 0;
				inline Driver& driver() {
					return this->m_driver;
				}
				inline const Driver& driver() const {
					return this->m_driver;
				}
				virtual bool finish_backup(const BackupId& backup_id) = 0;
				virtual BlockId get_block(const std::vector<uint8_t>& digest, const String& hash_algo, const KeyId& key) = 0;
				virtual FileId get_file(const Source::FileInfo& file_info, const HostId& host) = 0;
				virtual MetadataId get_metadata(const std::vector<uint8_t>& digest, const String& hash_algo) = 0;
				virtual bool has_block(const std::vector<uint8_t>& digest, const String& hash_algo, const KeyId& key) = 0;
				virtual BlockId insert_block(const std::vector<uint8_t>& block, const std::vector<uint8_t>& digest, const String& hash_algo, const KeyId& key) = 0;
				virtual FileId insert_file(const Source::FileInfo& file_info, const HostId& host) = 0;
				virtual MetadataId insert_metadata(const std::vector<uint8_t>& data, const std::vector<uint8_t>& digest, const String& hash_algo) = 0;
				virtual bool is_newer_or_not_exists(const Source::FileInfo& file_info, const HostId& host_id) = 0;
				virtual bool link_file_to_backup(const FileId& file_id, const BackupId& backup_id, const MetadataId& metadata_id) = 0;
				virtual bool link_file_to_block(const FileId& file_id, const BlockId& block_id, uint64_t sequence) = 0;
				virtual BackupId start_backup() = 0;
				virtual HostId synchronize_host(const Host& host) = 0;
				virtual KeyId synchronize_key(const Key& key) = 0;

			protected:
				Connection(Driver& driver);

			private:
				Driver& m_driver;
		};
	}
}

#endif
