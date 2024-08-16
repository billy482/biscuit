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

#ifndef __BISCUIT_WORKER_BACKUP_HPP__
#define __BISCUIT_WORKER_BACKUP_HPP__

#include "worker.hpp"

namespace YAML {
	class Node;
}

namespace Biscuit {
	namespace Db {
		class BackupId;
	}
	struct Option;

	namespace Worker {
		class Backup : public Worker {
			public:
				Backup(const Db::BackupId& backup_id);
				Backup(const Backup& backup);
				virtual ~Backup() = default;

				static int do_backup(const YAML::Node& config, const struct Option& options);
				virtual void run() override;

			private:
				uint16_t m_id;
				static uint16_t ms_ids;
				const Db::BackupId& m_backup_id;
				std::mutex m_lock;
				uint64_t m_current_file = 0;
				String m_current_path;
				uint64_t m_current_position = 0;
				uint64_t m_current_size = 0;
				static uint16_t ms_block_size;
				static const struct Checksum * ms_checksum;
		};
	}
}

#endif
