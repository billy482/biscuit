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
#include <spdlog/spdlog.h>
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


File::File(const QString& path) : m_root(path) {
	this->m_paths.push(QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware));
}


File * File::configure(const QString& path, const Node& node) {
	if (not node.IsMap())
		return nullptr;

	File * new_file = new File(path);

	const Node& include_patterns = node["include_patterns"];
	if (include_patterns.IsDefined() and include_patterns.IsSequence())
		for (YAML::const_iterator iter = include_patterns.begin(); iter != include_patterns.end(); iter++)
			new_file->m_include_pattern.append(QRegularExpression::fromWildcard(QString::fromStdString(iter->as<std::string>())));

	const Node& exclude_paths = node["exclude"];
	if (exclude_paths.IsDefined() and exclude_paths.IsSequence())
		for (YAML::const_iterator iter = exclude_paths.begin(); iter != exclude_paths.end(); iter++)
			new_file->m_exclude_path.append(QString::fromStdString(iter->as<std::string>()));

	const Node& exclude_patterns = node["exclude_patterns"];
	if (exclude_patterns.IsDefined() and exclude_patterns.IsSequence())
		for (YAML::const_iterator iter = exclude_patterns.begin(); iter != exclude_patterns.end(); iter++)
			new_file->m_exclude_pattern.append(QRegularExpression::fromWildcard(QString::fromStdString(iter->as<std::string>())));

	const Node& option = node["options"];
	if (option.IsDefined() and option.IsMap()) {
		const Node& exclude_other = option["exclude_other_filesystem"];
		if (exclude_other.IsDefined() and exclude_other.IsScalar())
			new_file->m_exclude_other_devices = exclude_other.as<bool>();

		const Node& exclude_if = option["exclude_if_present"];
		if (exclude_if.IsDefined() and exclude_if.IsSequence())
			for (YAML::const_iterator iter = exclude_if.begin(); iter != exclude_if.end(); iter++)
				new_file->m_exclude_dir_if.append(QString::fromStdString(iter->as<std::string>()));
	}

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

Biscuit::Host& File::host() {
	return this->m_host;
}

const Biscuit::Host& File::host() const {
	return this->m_host;
}

QIODevice * File::open(const FileInfo& file) {
	QFile * new_file = new QFile(file.path());
	if (new_file->open(QIODevice::ReadOnly))
		return new_file;
	else {
		delete new_file;
		return nullptr;
	}
}

FileInfo File::next() {
	auto logger = spdlog::get("core");

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
				logger->error("File: error while getting file information, path: {}, error: {}", raw_dir.data(), strerror(errno));

			QByteArray raw_file = file.absolutePath().toUtf8();
			int ret_file = lstat(raw_file.data(), &st_file);
			if (ret_file != 0)
				logger->error("File: error while getting file information, path: {}, error: {}", raw_file.data(), strerror(errno));

			if (ret_dir == 0 and ret_file == 0 and st_dir.st_dev != st_file.st_dev) {
				logger->info("File: skipping file {} because this file is on another device", raw_file.data());
				continue;
			}
		}

		if (file.isFile()) {
			if (this->m_include_pattern.size() > 0) {
				bool has_matched = false;
				for (auto iter = this->m_include_pattern.begin(); iter != this->m_include_pattern.end(); iter++) {
					QRegularExpressionMatch match = iter->match(file.fileName());
					if (match.hasMatch()) {
						has_matched = true;
						break;
					}
				}

				if (not has_matched) {
					logger->debug("Ignoring file: {}", file.filePath().toLocal8Bit().data());
					continue;
				}
			}

			if (this->m_exclude_pattern.size() > 0) {
				bool has_matched = false;
				for (auto iter = this->m_exclude_pattern.begin(); iter != this->m_exclude_pattern.end(); iter++) {
					QRegularExpressionMatch match = iter->match(file.absolutePath());
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
			for (auto iter = this->m_exclude_path.begin(); iter != this->m_exclude_path.end(); iter++) {
				QString& path = *iter;

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
