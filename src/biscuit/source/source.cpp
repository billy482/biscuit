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

#include <string>
#include <yaml-cpp/yaml.h>

//#include "file.hpp"
#include "source.hpp"
//#include "ssh.hpp"

using namespace Biscuit::Source;
using YAML::Node;


Source * Source::ms_first = nullptr;
Source * Source::ms_last = nullptr;

Source::Source(const Host& host) : m_host(host) {}

Source::~Source() {}


void Source::configure_options(const Node& options) {
	auto from_wildcard = [](const std::string& pattern) {
		std::string new_pattern = pattern;

		size_t pos = new_pattern.find("?");
		while (pos != std::string::npos) {
			new_pattern = new_pattern.replace(pos, 1, ".");
			pos = new_pattern.find("?");
		}

		pos = new_pattern.find("*");
		while (pos != std::string::npos) {
			new_pattern = new_pattern.replace(pos, 1, ".*");
			pos = new_pattern.find("*");
		}

		return std::regex(new_pattern);
	};

	const Node& include_patterns = options["include_patterns"];
	if (include_patterns.IsDefined() and include_patterns.IsSequence())
		for (YAML::const_iterator iter = include_patterns.begin(); iter != include_patterns.end(); iter++)
			this->m_include_pattern.push_back(from_wildcard(iter->as<std::string>()));

	const Node& exclude_paths = options["exclude"];
	if (exclude_paths.IsDefined() and exclude_paths.IsSequence())
		for (YAML::const_iterator iter = exclude_paths.begin(); iter != exclude_paths.end(); iter++)
			this->m_exclude_path.push_back(iter->as<std::string>().c_str());

	const Node& exclude_patterns = options["exclude_patterns"];
	if (exclude_patterns.IsDefined() and exclude_patterns.IsSequence())
		for (YAML::const_iterator iter = exclude_patterns.begin(); iter != exclude_patterns.end(); iter++)
			this->m_exclude_pattern.push_back(from_wildcard(iter->as<std::string>()));

	const Node& option = options["options"];
	if (option.IsDefined() and option.IsMap()) {
		const Node& exclude_other = option["exclude_other_filesystem"];
		if (exclude_other.IsDefined() and exclude_other.IsScalar())
			this->m_exclude_other_devices = exclude_other.as<bool>();

		const Node& exclude_if = option["exclude_if_present"];
		if (exclude_if.IsDefined() and exclude_if.IsSequence())
			for (YAML::const_iterator iter = exclude_if.begin(); iter != exclude_if.end(); iter++)
				this->m_exclude_dir_if.push_back(iter->as<std::string>().c_str());
	}
}

Source * Source::first_source() {
	return Source::ms_first;
}

bool Source::parse(const Node& config) {
	if (not config.IsSequence())
		return false;

	for (YAML::const_iterator iter = config.begin(); iter != config.end(); iter++) {
		Source * src = nullptr;

		const Node& file = *iter;
		if (not file.IsMap())
			return false;

		const Node& option = file["options"];
		if (option.IsDefined()) {
			if (not option.IsMap())
				return false;

			// const Node& ssh = option["ssh"];
			// if (ssh.IsDefined() and ssh.IsMap())
			//	src = Ssh::configure(file);
		}

		//if (src == nullptr)
		//	src = File::configure(file);

		if (src == nullptr)
			return false;

		if (Source::ms_first == nullptr)
			Source::ms_first = Source::ms_last = src;
		else {
			src->m_previous = Source::ms_last;
			Source::ms_last->m_next = src;
			Source::ms_last = src;
		}
	}

	return true;
}
