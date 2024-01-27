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

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "driver.hpp"
#include "sqlite/driver.hpp"

using namespace Biscuit::Db;
using Biscuit::Db::Sqlite::SqliteDriver;
using YAML::Node;

Driver * Driver::ms_instance = nullptr;

Driver::Driver(const QString& name) : m_name(name) {}

Driver::~Driver() {
	if (this == Driver::ms_instance)
		Driver::ms_instance = nullptr;
}


bool Driver::configure(const Node& node) {
	auto logger = spdlog::get("database");
	const Node& driver = node["driver"];
	if (not driver.IsScalar()) {
		logger->error("Error: driver is not a scalar");
		return false;
	}

	QString str_driver(driver.as<std::string>().c_str());
	if (str_driver == "sqlite") {
		logger->debug("Configuring sqlite driver");
		Driver::ms_instance = SqliteDriver::configure(node);
	} else {
		logger->error("Error: driver \"{}\" not available", str_driver.toUtf8().data());
		return false;
	}

	return true;
}

Driver * Driver::get() {
	return Driver::ms_instance;
}
