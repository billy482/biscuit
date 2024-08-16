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

#include "file-id.hpp"

using namespace Biscuit::Db;

FileId::FileId(SqlStatus status) : SqlResult(status) {}

FileId::FileId(SqlStatus status, const std::any& value) : SqlResult(status, value) {}

FileId::FileId(SqlStatus status, std::any&& value) : SqlResult(status, std::move(value)) {}

FileId::FileId(const FileId& file_id) : SqlResult(file_id) {}

FileId::FileId(FileId&& file_id) : SqlResult(file_id) {}


FileId& FileId::operator=(const FileId& file_id) {
	this->copy(file_id);
	return *this;
}

FileId& FileId::operator=(FileId&& file_id) {
	this->move(std::move(file_id));
	return *this;
}

