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

#ifndef __BISCUIT_KEY_HPP__
#define __BISCUIT_KEY_HPP__

#include <botan/auto_rng.h>
#include <botan/pk_keys.h>
#include <cstdint>
#include <memory>
#include <QtCore/QFileInfo>

class QByteArray;

namespace YAML {
	class Node;
}

namespace Biscuit {
	class Key {
		public:
			Key() = default;
			Key(const QFileInfo& filename);
			Key(Key&& key);
			~Key() = default;

			static bool configure(const YAML::Node& config);
			QByteArray decrypt(const QByteArray& block) const;
			QByteArray encrypt(const QByteArray& block) const;
			QString fingerprint() const;
			static Key& get();
			uint16_t key_length() const;
			bool open_for_decrypt();
			bool open_for_encrypt();

			Key& operator =(Key&& key);

		private:
			QFileInfo m_filename;
			mutable Botan::AutoSeeded_RNG m_rng;
			std::unique_ptr<Botan::Private_Key> m_private_key = nullptr;
			std::unique_ptr<Botan::Public_Key> m_public_key = nullptr;
			static Key ms_key;
	};
}

#endif
