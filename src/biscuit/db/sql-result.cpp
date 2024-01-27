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

#include "sql-result.hpp"

using namespace Biscuit::Db;

SqlResult::SqlResult(SqlStatus status) : m_status(status), m_value() {}

SqlResult::SqlResult(SqlStatus status, const QVariant& value) : m_status(status), m_value(value) {}

SqlResult::SqlResult(SqlStatus status, QVariant&& value) : m_status(status), m_value(std::move(value)) {}

SqlResult::SqlResult(const SqlResult& result) : m_status(result.m_status), m_value(result.m_value) {}

SqlResult::SqlResult(SqlResult&& result) : m_status(result.m_status), m_value(std::move(result.m_value)) {}


void SqlResult::copy(const SqlResult& src) {
	this->m_status = src.m_status;
	this->m_value = src.m_value;
}

void SqlResult::move(SqlResult&& src) {
	this->m_status = src.m_status;
	this->m_value = std::move(src.m_value);
}
