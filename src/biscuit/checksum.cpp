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

#include <QtCore/QString>

#include "checksum.hpp"

const Biscuit::Checksum * Biscuit::Checksum::find(const QString& name, bool& found) {
	static struct Checksum algos[] = {
		{ "md4",      QCryptographicHash::Md4 },
		{ "md5",      QCryptographicHash::Md5 },
		{ "sha1",     QCryptographicHash::Sha1 },
		{ "sha224",   QCryptographicHash::Sha224 },
		{ "sha256",   QCryptographicHash::Sha256 },
		{ "sha384",   QCryptographicHash::Sha384 },
		{ "sha512",   QCryptographicHash::Sha512 },
		{ "sha3-224", QCryptographicHash::Sha3_224 },
		{ "sha3-256", QCryptographicHash::Sha3_256 },
		{ "sha3-384", QCryptographicHash::Sha3_384 },
		{ "sha3-512", QCryptographicHash::Sha3_512 },

		{ nullptr, QCryptographicHash::Md4 }
	};

	QByteArray raw_name = name.toUtf8();
	for (struct Checksum * ptr = algos; ptr->name != nullptr; ptr++)
		if (raw_name == ptr->name) {
			found = true;
			return ptr;
		}

	found = false;
	return algos + 2;
}
