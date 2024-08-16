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
#include <string.h>
#include <sys/stat.h>
#include <yaml-cpp/yaml.h>

#include "file.hpp"

using namespace Biscuit::Source;
using YAML::Node;


File::File(const std::string& path) : Source(Host::localhost()), m_root(std::filesystem::absolute(path)), m_logger(spdlog::get("core")) {
	this->scan_directory(this->m_root);
}


File * File::configure(const Node& file) {
	const Node& path = file["path"];
	if (not path.IsDefined() or not path.IsScalar())
		return nullptr;

	File * new_file = new File(path.as<std::string>());
	new_file->configure_options(file);

	return new_file;
}

nlohmann::json File::get_metadata(const std::filesystem::path& file_info) {
	struct stat st_file;
	int ret = lstat(file_info.c_str(), &st_file);
	if (ret == 0)
		return nullptr;

	return {
		{"common", {
			{"modified time", st_file.st_mtim.tv_sec},
			{"size", std::filesystem::file_size(file_info)}
		}},
		{"unix", {
			{"gid", st_file.st_gid},
			{"uid", st_file.st_uid}
		}}
	};
}

FileInfo File::next(uint16_t, uint16_t) {
	this->m_lock.lock();

	while (not this->m_paths.size() == 0) {
		std::list<FileInfo> files = this->m_paths.back();
		if (files.size() == 0) {
			this->m_paths.pop_back();
			continue;
		}

		FileInfo file = files.front();
		files.pop_front();

		if (this->m_exclude_other_devices) {
			struct stat st_dir, st_file;

			std::filesystem::path path = std::filesystem::path(std::string(file.path()));
			std::filesystem::path parent_dir = path.parent_path();
			int ret_dir = lstat(parent_dir.c_str(), &st_dir);
			if (ret_dir != 0)
				this->m_logger->error("File: error while getting file information, path: {}, error: {}", parent_dir.string(), strerror(errno));

			std::filesystem::path absolute_path = std::filesystem::absolute(path);
			int ret_file = lstat(absolute_path.c_str(), &st_file);
			if (ret_file != 0)
				this->m_logger->error("File: error while getting file information, path: {}, error: {}", absolute_path.string(), strerror(errno));

			if (ret_dir == 0 and ret_file == 0 and st_dir.st_dev != st_file.st_dev) {
				this->m_logger->info("File: skipping file {} because this file is on another device", absolute_path.string());
				continue;
			}
		}

		if (file.is_file()) {
			if (this->m_include_pattern.size() > 0) {
				bool has_matched = false;
				for (const std::regex& expression : this->m_include_pattern) {
					std::cmatch match;
					if (std::regex_match(static_cast<const char *>(file.path()), match, expression)) {
						has_matched = true;
						break;
					}
				}

				if (not has_matched) {
					this->m_logger->debug("Ignoring file: {}", static_cast<const char *>(file.path()));
					continue;
				}
			}

			if (this->m_exclude_pattern.size() > 0) {
				bool has_matched = false;
				for (const std::regex& expression : this->m_exclude_pattern) {
					std::cmatch match;
					if (std::regex_match(static_cast<const char *>(file.path()), match, expression)) {
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
			const std::filesystem::path file_path = std::filesystem::absolute(std::filesystem::path(std::string(file.path())));

			for (const String& path : this->m_exclude_path) {
				if (path.starts_with('/')) {
					const std::string sub_path = file_path.string().substr(this->m_root.string().length());

					if (sub_path == static_cast<const char *>(path)) {
						has_matched = true;
						break;
					}
				} else if (static_cast<const char *>(path) == file_path.filename().string()) {
					has_matched = true;
					break;
				}
			}

			if (has_matched)
				continue;
		}
		
		if (file.is_dir()) {
			this->scan_directory(std::filesystem::path(std::string(file.path())));

			bool found = false;
			if (this->m_exclude_dir_if.size() > 0)
				for (std::list<FileInfo>::iterator iter_file = files.begin(); iter_file != files.end() and not found; iter_file++)
					for (std::list<String>::iterator iter = this->m_exclude_dir_if.begin(); iter != this->m_exclude_dir_if.end() and not found; iter++) {
						const std::filesystem::path file_name(static_cast<const char *>(iter_file->path()));
						found = file_name.filename() == static_cast<const char *>(*iter);
					}

			/* TODO: finish this
			if (not found and files.size() > 0)
				this->m_paths.push(files);
			*/
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
