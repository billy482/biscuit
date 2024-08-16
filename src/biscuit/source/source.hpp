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

#ifndef __BISCUIT_SOURCE_SOURCE_HPP__
#define __BISCUIT_SOURCE_SOURCE_HPP__

#include <list>
#include <mutex>
#include <regex>

#include "../host.hpp"

namespace YAML {
	class Node;
}

namespace Biscuit {
	class Host;

	namespace Source {
		class FileInfo;
		class StreamReader;

		class Source {
			public:
				static Source * first_source();
				inline const Host& host() const {
					return this->m_host;
				}
				virtual StreamReader * open(const FileInfo& file, uint16_t worker) = 0;
				virtual FileInfo next(uint16_t worker, uint16_t total_workers) = 0;
				inline Source * next_source() {
					return this->m_next;
				}
				static bool parse(const YAML::Node& node);
				inline Source * previous_source() {
					return this->m_previous;
				}

			protected:
				Source(const Host& host);
				virtual ~Source();

				void configure_options(const YAML::Node& node);

				Host m_host;
				std::mutex m_lock;
				std::list<std::regex> m_include_pattern;
				std::list<std::regex> m_exclude_pattern;
				std::list<String> m_exclude_path;
				std::list<String> m_exclude_dir_if;
				bool m_exclude_other_devices = false;

			private:
				static Source * ms_first;
				static Source * ms_last;
				Source * m_next = nullptr;
				Source * m_previous = nullptr;
		};
	}
}

#endif
