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

#include <errno.h>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <string.h>
#include <sys/stat.h>
#include <yaml-cpp/yaml.h>

#include "file.hpp"
#include "file-info.hpp"

using namespace Biscuit::Source;
using YAML::Node;


File::File(const QString& path) : Source(Host::localhost()), m_root(path), m_logger(spdlog::get("core")) {
	this->m_paths.push(QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware));
}


File * File::configure(const Node& file) {
	const Node& path = file["path"];
	if (not path.IsDefined() or not path.IsScalar())
		return nullptr;

	const QString filename(path.as<std::string>().c_str());

	File * new_file = new File(filename);
	new_file->configure_options(file);

	return new_file;
}

QJsonDocument File::get_metadata(const QFileInfo& file_info) {
	QJsonObject md_common;
	md_common.insert("size", QJsonValue(file_info.size()));

	QJsonObject md_unix;
	md_unix.insert("owner", QJsonValue(file_info.owner()));
	md_unix.insert("group", QJsonValue(file_info.group()));

	QJsonObject metadata;
	metadata.insert("common", md_common);
	metadata.insert("unix", md_unix);

	return QJsonDocument(metadata);
}

FileInfo File::next(uint16_t, uint16_t) {
	this->m_lock.lock();

	while (not this->m_paths.isEmpty()) {
		QFileInfoList& files = this->m_paths.top();
		if (files.size() == 0) {
			this->m_paths.pop();
			continue;
		}

		QFileInfo file = files.first();
		files.pop_front();

		if (this->m_exclude_other_devices) {
			struct stat st_dir, st_file;

			QByteArray raw_dir = file.dir().absolutePath().toUtf8();
			int ret_dir = lstat(raw_dir.data(), &st_dir);
			if (ret_dir != 0)
				this->m_logger->error("File: error while getting file information, path: {}, error: {}", raw_dir.data(), strerror(errno));

			QByteArray raw_file = file.absolutePath().toUtf8();
			int ret_file = lstat(raw_file.data(), &st_file);
			if (ret_file != 0)
				this->m_logger->error("File: error while getting file information, path: {}, error: {}", raw_file.data(), strerror(errno));

			if (ret_dir == 0 and ret_file == 0 and st_dir.st_dev != st_file.st_dev) {
				this->m_logger->info("File: skipping file {} because this file is on another device", raw_file.data());
				continue;
			}
		}

		if (file.isFile()) {
			if (this->m_include_pattern.size() > 0) {
				bool has_matched = false;
				for (const QRegularExpression& expression : this->m_include_pattern) {
					QRegularExpressionMatch match = expression.match(file.fileName());
					if (match.hasMatch()) {
						has_matched = true;
						break;
					}
				}

				if (not has_matched) {
					this->m_logger->debug("Ignoring file: {}", file.filePath().toLocal8Bit().data());
					continue;
				}
			}

			if (this->m_exclude_pattern.size() > 0) {
				bool has_matched = false;
				for (const QRegularExpression& expression : this->m_exclude_pattern) {
					QRegularExpressionMatch match = expression.match(file.fileName());
					if (match.hasMatch()) {
						has_matched = true;
						break;
					}
				}

				if (has_matched)
					continue;
			}
		}

		if (this->m_exclude_path.size() > 0) {
			bool has_matched = false;
			for (const QString& path : this->m_exclude_path) {
				if (path.startsWith('/')) {
					QString sub_path = file.absoluteFilePath().mid(this->m_root.absoluteFilePath().length());
					if (sub_path == path) {
						has_matched = true;
						break;
					}
				} else if (path == file.fileName()) {
					has_matched = true;
					break;
				}
			}

			if (has_matched)
				continue;
		}
		
		if (file.isDir()) {
			QFileInfoList files = QDir(file.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware);

			bool found = false;
			if (this->m_exclude_dir_if.size() > 0)
				for (auto iter_file = files.begin(); iter_file != files.end() and not found; iter_file++)
					for (auto iter = this->m_exclude_dir_if.begin(); iter != this->m_exclude_dir_if.end() and not found; iter++)
						found = iter_file->fileName() == *iter;

			if (not found and files.size() > 0)
				this->m_paths.push(files);
		}

		this->m_lock.unlock();

		return FileInfo(file, this->get_metadata(file));
	}

	this->m_lock.unlock();
	return FileInfo();
}

QIODevice * File::open(const FileInfo& file, uint16_t) {
	QFile * new_file = new QFile(file.path());
	if (new_file->open(QIODevice::ReadOnly))
		return new_file;
	else {
		delete new_file;
		return nullptr;
	}
}
