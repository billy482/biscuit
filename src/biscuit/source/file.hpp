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

#ifndef __BISCUIT_SOURCE_FILE_HPP__
#define __BISCUIT_SOURCE_FILE_HPP__

#include <QtCore/QFileInfo>
#include <QtCore/QStack>
#include <spdlog/spdlog.h>

#include "source.hpp"

class QJsonDocument;

namespace Biscuit {
	namespace Source {
		class File : public Source {
			public:
				virtual ~File() = default;

				static File * configure(const YAML::Node& file);
				virtual FileInfo next(uint16_t worker, uint16_t total_workers) override;
				virtual QIODevice * open(const FileInfo& file, uint16_t worker) override;

			private:
				File(const QString& path);

				QJsonDocument get_metadata(const QFileInfo& info);

				QFileInfo m_root;
				QStack<QFileInfoList> m_paths;
				std::shared_ptr<spdlog::logger> m_logger;
		};
	}
}

#endif
