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

#include <QtCore/QDir>
#include <string>
#include <yaml-cpp/yaml.h>

#include "file.hpp"
#include "source.hpp"

using namespace Biscuit::Source;
using YAML::Node;


Source * Source::ms_first = nullptr;
Source * Source::ms_last = nullptr;

Source::~Source() {}


Source * Source::first_source() {
	return Source::ms_first;
}

bool Source::parse(const Node& config) {
	if (not config.IsMap())
		return false;

	for (YAML::const_iterator iter = config.begin(); iter != config.end(); iter++) {
		const std::string path = iter->first.as<std::string>();
		Source * src = File::configure(path.c_str(), iter->second);

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
