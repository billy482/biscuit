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

#ifndef __BISCUIT_SOURCE_FILEINFO_HPP__
#define __BISCUIT_SOURCE_FILEINFO_HPP__

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QString>

class QFileInfo;

namespace Biscuit {
	namespace Source {
		enum class FileType {
			File,
			Directory,
			Unknown
		};

		class FileInfo {
			public:
				FileInfo() = default;
				FileInfo(const QString& path, const QDateTime& modified_time, FileType type, uint64_t file_size, const QJsonObject& metadata);
				FileInfo(const QFileInfo& info, const QJsonObject& metadata);
				FileInfo(const QFileInfo& info, QJsonDocument&& metadata);
				FileInfo(const FileInfo& info);

				static FileType from(const QFileInfo& file_info);
				inline uint64_t file_size() const {
					return this->m_file_size;
				}
				inline bool is_dir() const {
					return this->m_type == FileType::Directory;
				}
				inline bool is_file() const {
					return this->m_type == FileType::File;
				}
				inline bool is_invalid() const {
					return this->m_is_invalid;
				}
				inline const QJsonDocument& metadata() const {
					return this->m_metadata;
				}
				inline const QDateTime& modified_time() const {
					return this->m_modified_time;
				}
				inline const QString& path() const {
					return this->m_path;
				}
				inline FileType type() const {
					return this->m_type;
				}

				FileInfo& operator =(const FileInfo& info);

			private:
				QString m_path;
				QDateTime m_modified_time;
				FileType m_type;
				uint64_t m_file_size = 0;
				QJsonDocument m_metadata;
				bool m_is_invalid = true;
		};
	}
}

#endif
