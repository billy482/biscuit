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

#include <QtCore/QFileInfo>

#include "file-info.hpp"

using namespace Biscuit::Source;

FileInfo::FileInfo(const QString& path, const QDateTime& modified_time, FileType type, uint64_t file_size, const QJsonObject& metadata) : m_path(path), m_modified_time(modified_time), m_type(type), m_file_size(file_size), m_metadata(metadata), m_is_invalid(false) {}

FileInfo::FileInfo(const QString& path, const QDateTime& modified_time, FileType type, uint64_t file_size, QJsonDocument&& metadata) : m_path(path), m_modified_time(modified_time), m_type(type), m_file_size(file_size), m_metadata(std::move(metadata)), m_is_invalid(false) {}

FileInfo::FileInfo(const QFileInfo& info, const QJsonObject& metadata) : m_path(info.absoluteFilePath()), m_modified_time(info.lastModified()), m_type(FileInfo::from(info)), m_file_size(info.size()), m_metadata(metadata), m_is_invalid(not info.exists()) {}

FileInfo::FileInfo(const QFileInfo& info, QJsonDocument&& metadata) : m_path(info.absoluteFilePath()), m_modified_time(info.lastModified()), m_type(FileInfo::from(info)), m_file_size(info.size()), m_metadata(std::move(metadata)), m_is_invalid(not info.exists()) {}

FileInfo::FileInfo(const FileInfo& info) : m_path(info.m_path), m_modified_time(info.m_modified_time), m_type(info.m_type), m_file_size(info.m_file_size), m_metadata(info.m_metadata), m_is_invalid(info.m_is_invalid) {}


FileType FileInfo::from(const QFileInfo& file_info) {
	if (file_info.isFile())
		return FileType::File;
	else if (file_info.isDir())
		return FileType::Directory;
	else
		return FileType::Unknown;
}


FileInfo& FileInfo::operator=(const FileInfo& info) {
	this->m_path = info.m_path;
	this->m_modified_time = info.m_modified_time;
	this->m_type = info.m_type;
	this->m_file_size = info.m_file_size;
	this->m_metadata = info.m_metadata;
	this->m_is_invalid = info.m_is_invalid;
	return *this;
}

bool FileInfo::operator<(const FileInfo& info) const {
	return this->m_path < info.m_path;
}
