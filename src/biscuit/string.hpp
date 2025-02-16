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
*  Copyright (C) 2025, Guillaume Clercin <guillaume.clercin@billy482.net>   *
\***************************************************************************/

#ifndef __BISCUIT_STRING_HPP__
#define __BISCUIT_STRING_HPP__

// va_list
#include <cstdarg>
// *int*_t
#include <cstdint>

namespace Biscuit {
	class StringPrivate;

	class String {
		public:
			String(const char * string = nullptr);
			String(const String& string);
			~String();

			void clear();
			static int8_t compare(const String& a, const String& b);
			uint32_t count(const String& str, uint32_t offset = 0) const;
			static uint32_t decode_from_utf8(const char * string, bool escape = false);
			static uint32_t decode_from_utf8(const char * string, uint8_t& length, bool escape = false);
			static uint8_t decode_length_from_utf8(const char * string, bool escape = false);
			static int8_t encode_to_utf8(char * string, uint32_t length, uint32_t unicode, bool null = true);
			static int8_t encode_to_utf8(char * string, uint32_t length, bool unescaped, uint32_t unicode, bool null = true);
			bool ends_with(const String& end) const;
			int32_t find(const String& str, uint32_t offset = 0) const;
			uint32_t get(uint32_t offset) const;
			inline uint64_t hash() const {
				return String::hash(*this);
			}
			static uint64_t hash(const char * string);
			static uint64_t hash(const String& string);
			inline bool is_null() const {
				return this->mString_is_null;
			}
			uint32_t length() const;
			String middle_ellipsis(uint32_t length, const String& middle = String("…")) const;
			String replace(const String& old_str, const String& new_str) const;
			static String sprintf(const char * format, ...) __attribute__((format(printf, 1, 2)));
			bool starts_with(char begin) const;
			bool starts_with(const String& begin) const;
			String substring(int32_t offset) const;
			String substring(int32_t offset, uint32_t length) const;
			uint32_t utf8_length() const;
			static uint8_t utf8_length(uint32_t unicode, bool unescape);
			static String vsprintf(const char * format, va_list args);

			String& operator =(const char * string);
			String& operator =(const String& string);
			String& operator +=(const String& string);
			String operator +(const String& string) const;
			inline bool operator ==(const String& b) const {
				return String::compare(*this, b) == 0;
			}
			inline bool operator !=(const String& b) const {
				return String::compare(*this, b) != 0;
			}
			inline bool operator <(const String& b) const {
				return String::compare(*this, b) < 0;
			}
			inline bool operator <=(const String& b) const {
				return String::compare(*this, b) <= 0;
			}
			inline bool operator >(const String& b) const {
				return String::compare(*this, b) > 0;
			}
			inline bool operator >=(const String& b) const {
				return String::compare(*this, b) >= 0;
			}
			operator const char *() const;

		private:
			String(StringPrivate * string);

			void discard_cache();

			mutable char * mString_cache = nullptr;
			StringPrivate * mString_data = nullptr;
			bool mString_is_null = true;
	};
}

#endif
